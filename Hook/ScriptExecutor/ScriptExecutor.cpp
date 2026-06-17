/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: ScriptExecutor.cpp
// Started by: Hattozo
// Started on: 6/14/2026
// Description: A working script executor!
// Skidded from Xeno because I'm not intelligent
// You may ask, why is there a script executor here? Because of the plugins system in noobWarrior, which allows developers to insert models into the DataModel.
// That sounds like a terrible idea, but trust me, I'm all for a moddable experience.
// Besides, if you're that concerned, write your god damn code to never trust the client. Geez.
#include "ScriptExecutor.h"
#include "Instance.h"

#include <psapi.h>
#include <shlobj.h>
#include <regex>
#include <filesystem>
#include <fstream>

uintptr_t NoobHook::ScriptExecutor::gDataModelAddress = 0;

uintptr_t NoobHook::ScriptExecutor::FindDataModelAddress() {
	if (gDataModelAddress != 0)
		return gDataModelAddress;

	WCHAR* path;
	SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &path);
	std::filesystem::path baseDir(path);
	CoTaskMemFree(path);

	std::filesystem::path logsDir(baseDir / "Roblox" / "logs");
	if (!std::filesystem::is_directory(logsDir)) {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Roblox logs directory not found");
		return 0;
	}

	std::vector<std::filesystem::path> logFiles;

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(logsDir)) {
		std::filesystem::path path = entry.path();
		if (entry.is_regular_file() && path.extension() == ".log" && path.string().contains("Player"))
			logFiles.push_back(path);
	}

	if (logFiles.empty()) {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", "No log files found in Roblox logs directory");
		return 0;
	}

	std::sort(logFiles.begin(), logFiles.end(), [](const auto& x, const auto& y) {
		return std::filesystem::last_write_time(x) > std::filesystem::last_write_time(y);
	});	
	
	std::filesystem::path logFileDir = logFiles.front();
	std::ifstream logFile(logFileDir);
	if (!logFile.is_open()) {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", std::string("Failed to open log file: " + logFileDir.string()).c_str());
		return 0;
	}

	std::stringstream buffer;
	buffer << logFile.rdbuf();
	std::string content = buffer.str();

	std::regex regex(R"(\bSurfaceController\[_:1\]::start dataModel\((.*?)\))");
	std::smatch match;
	
	std::string addrStr;

	NoobHook::Out("ScriptExecutor", "%s", content.c_str());
	if (std::regex_search(content, match, regex)) {
		if (match.size() <= 1) {
			NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Failed to find regex match for DataModel address in log file");
			return 0;
		}
		addrStr = match[1].str();
	} else {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Failed to find regex match for DataModel address in log file");
		return 0;
	}
	if (addrStr.empty()) {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Failed to find DataModel address in log file");
		return 0;
	}
	NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Found DataModel address string in log file: %s", addrStr.c_str());

	uintptr_t addr = strtoull(addrStr.c_str(), nullptr, 16);
	if (NoobHook::ReadPrimitive<uintptr_t>(addr) == 0) {
		NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Failed to read valid DataModel address from log file");
		return 0;
	}
	gDataModelAddress = addr;
	NoobHook::Out("ScriptExecutor::FindDataModelAddress", "Found DataModel address: 0x%08X", static_cast<uintptr_t>(addr));
	return addr;
}

std::vector<uintptr_t> NoobHook::ScriptExecutor::GetChildrenAddresses(uintptr_t address) {
	std::vector<uintptr_t> children;
	{
		uintptr_t childrenPtr = NoobHook::ReadPrimitive<uintptr_t>(address + NoobHook::ScriptExecutor::Offsets::Children);
		if (childrenPtr == 0)
			return children;

		uintptr_t childrenStart = NoobHook::ReadPrimitive<uintptr_t>(childrenPtr);
		uintptr_t childrenEnd = NoobHook::ReadPrimitive<uintptr_t>(childrenPtr + 0x8) + 1;

		for (uintptr_t childAddress = childrenStart; childAddress < childrenEnd; childAddress += 0x10) {
			uintptr_t childPtr = NoobHook::ReadPrimitive<uintptr_t>(childAddress);
			if (childPtr != 0)
				children.push_back(childPtr);
		}
	}
	return children;
}

std::string NoobHook::ScriptExecutor::ReadRobloxString(uintptr_t address) {
	uint64_t stringCount = NoobHook::ReadPrimitive<uint64_t>(address + 0x10);

	if (stringCount > 15000 || stringCount <= 0)
		return "";

	if (stringCount > 15)
		address = NoobHook::ReadPrimitive<uintptr_t>(address);

	std::string buffer;
	buffer.resize(stringCount);

	MEMORY_BASIC_INFORMATION bi;
	VirtualQueryEx(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), &bi, sizeof(bi));

	NtReadVirtualMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), buffer.data(), (ULONG)stringCount, nullptr);

	PVOID baddr = bi.AllocationBase;
	SIZE_T size = bi.RegionSize;
	NtUnlockVirtualMemory(GetCurrentProcess(), &baddr, &size, 1);

	return buffer;
}

void NoobHook::ScriptExecutor::Install() {
	HMODULE ntdll = LoadLibraryA("ntdll.dll");
	if (!ntdll) {
		Out("ScriptExecutor", "Could not load ntdll.dll");
		return;
	}
	NTDLL_INIT_FCNS(ntdll);

	// Not really the best way to wait for the client to load
	PROCESS_MEMORY_COUNTERS memory_counter;
	K32GetProcessMemoryInfo(GetCurrentProcess(), &memory_counter, sizeof(memory_counter));
	while (memory_counter.WorkingSetSize < 150000000) {
		K32GetProcessMemoryInfo(GetCurrentProcess(), &memory_counter, sizeof(memory_counter));
		Sleep(1000);
	}

	uintptr_t DataModelAddress = FindDataModelAddress();
	if (DataModelAddress == 0) {
		NoobHook::Out("ScriptExecutor", "Failed to find DataModel address!");
		return;
	}
	Instance inst = Instance(NoobHook::ScriptExecutor::gDataModelAddress);
	Out("ScriptExecutor", "DataModel address: 0x%08X", static_cast<uintptr_t>(inst.GetAddress()));
	//Out("ScriptExecutor", "DataModel name: %s", inst.GetName().c_str());
	Out("ScriptExecutor", "ScriptExecutor installed successfully!");
}
