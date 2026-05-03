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
// File: Engine.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description: Implementation for all methods related to handling Roblox clients
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Engine.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/NetClient.h>
#include <NoobWarrior/Paths.h>

#include <curl/curl.h>
#include <zip.h>
#include <zipconf.h>

#include <filesystem>
#include <thread>
#include <set>
#include <codecvt>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

using namespace NoobWarrior;

nlohmann::json Core::GetEngineManifest() {
    nlohmann::json manifest;
    try {
        std::ifstream stream(GetUserDataDir() / NW_PATH_ENGINES / "engines.json");
        std::string str;
        std::string line;
        while (std::getline(stream, line)) {
            str += line + "\n";
        }
        stream.close();
        manifest = nlohmann::json::parse(str);
    } catch (nlohmann::json::exception &e) {
        manifest = nlohmann::json::array();
    }
    return manifest;
}

std::vector<Engine> Core::GetInstalledEngines() {
    std::vector<Engine> engines;
    if (!std::filesystem::exists(GetUserDataDir() / NW_PATH_ENGINES))
        return engines;

    nlohmann::json manifest = GetEngineManifest();
    for (auto &item : manifest.items()) {
        Engine engine {};
        auto engineJson = item.value();
        if (engineJson.contains("Os") && engineJson["Os"].is_string()) {
            std::string osStr = engineJson["Os"].get<std::string>();
            if (osStr.compare("Windows") == 0)
                engine.Os = EngineOs::Windows;
            else if (osStr.compare("Mac") == 0)
                engine.Os = EngineOs::Mac;
            else if (osStr.compare("Linux") == 0)
                engine.Os = EngineOs::Linux;
            else if (osStr.compare("Android") == 0)
                engine.Os = EngineOs::Android;
            else if (osStr.compare("Ios") == 0)
                engine.Os = EngineOs::Ios;
        }

        if (engineJson.contains("Arch") && engineJson["Arch"].is_string()) {
            std::string archStr = engineJson["Arch"].get<std::string>();
            if (archStr.compare("x86") == 0)
                engine.Architecture = EngineArchitecture::x86;
            else if (archStr.compare("x86_64") == 0)
                engine.Architecture = EngineArchitecture::x86_64;
        }

        if (engineJson.contains("Type") && engineJson["Type"].is_string()) {
            std::string typeStr = engineJson["Type"].get<std::string>();
            if (typeStr.compare("Roblox") == 0)
                engine.Type = EngineType::Roblox;
        }

        if (engineJson.contains("Version"))
            engine.Version = engineJson["Version"].get<std::string>();

        if (engineJson.contains("Hash"))
            engine.Hash = engineJson["Hash"].get<std::string>();

        engines.push_back(engine);
    }

    return engines;
}

std::vector<Engine> Core::GetAllEngines() {
    return {};
}

std::filesystem::path Core::GetEngineDirectory(const Engine &engine) {
    for (auto &item : GetEngineManifest().items()) {
        auto engineJson = item.value();
        Out("IsEngineInManifest", "{}", engineJson.dump());

        if (EngineSideAsString(engine.Side).compare(engineJson["Side"]) != 0)
            continue;

        // if you give it a hash it knows where to look
        if (engineJson.contains("Hash") && engineJson["Hash"].is_string()) {
            if (engine.Hash.compare(engineJson["Hash"].get<std::string>()) == 0) {
                return GetUserDataDir() / NW_PATH_ENGINES / engine.Hash;
            }
        }

        // if you give it a version but not a hash, it will try finding that
        if (engineJson.contains("Version") && engineJson["Version"].is_string()) {
            if (engine.Version.compare(engineJson["Version"].get<std::string>()) == 0
                && engineJson.contains("Hash")
                && engineJson["Hash"].is_string()) {
                return GetUserDataDir() / NW_PATH_ENGINES / engineJson["Hash"].get<std::string>();
            }
        }
    }
    return {};
}

void Core::DiscoverEngines() {

}

bool Core::IsEngineInManifest(const Engine &engine) {
    for (auto &item : GetEngineManifest().items()) {
        auto engineJson = item.value();
        if (engineJson.contains("Version") && engineJson["Version"].is_string()) {
            if (engine.Version.compare(engineJson["Version"].get<std::string>()) == 0)
                return true;
        }

        if (engineJson.contains("Hash") && engineJson["Hash"].is_string()) {
            if (engine.Hash.compare(engineJson["Hash"].get<std::string>()) == 0)
                return true;
        }
    }
    return false;
}

void Core::DownloadAndInstallEngine(const Engine &engine, std::shared_ptr<std::vector<std::shared_ptr<Transfer>>> &transfers, std::shared_ptr<std::function<void(EngineInstallState, CURLcode, size_t, size_t)>> callback) {
    nlohmann::json index;

    bool foundEngine = false;

    if (!foundEngine) {
        Out("Download", "Failed to find a client");
        (*callback)(EngineInstallState::Failed, CURLE_OK, 0, 0);
    }
}

#if defined(_WIN32)
static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
}
#endif

EngineLaunchResponse Core::LaunchProcessThroughInjector(EngineArchitecture arch, const std::filesystem::path &filePath, EngineStartParameters params) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring ipStrW = converter.from_bytes(params.Ip);
    const std::filesystem::path &injectorPath = GetInstallationDir() / (arch == EngineArchitecture::x86_64 ? "noobhook_x86-64_injector.exe" : "noobhook_x86_injector.exe");
    if (!std::filesystem::exists(injectorPath)) {
        Out("Inject", "Failed to create injector process: Injector process doesn't exist!");
        return EngineLaunchResponse::FailedToCreateProcess;
    }

    std::vector<std::string> args;
    args.push_back(injectorPath.string());
    args.push_back("--file");
#if defined(_WIN32)
    args.push_back(filePath.string());
#else
    args.push_back(GetWinePath(filePath));
#endif
    if (!params.Ip.empty()) {
        args.push_back("--ip");
        args.push_back(params.Ip);
    }
    if (params.Port.has_value()) {
        args.push_back("--port");
        args.push_back(std::to_string(params.Port.value()));
    }

    std::string argsStr;
    for (int i = 0; i < argsStr.size(); i++) {
        argsStr += argsStr[i];
        if (i < argsStr.size() - 1) {
            argsStr += " ";
        }
    }
    Out("Inject", "Launching process \"{}\"", argsStr);
#if defined(_WIN32)
    PROCESS_INFORMATION pi {};
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    if (!CreateProcessA(nullptr, const_cast<char*>(argsStr.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        Out("Inject", "Failed to create injector process: {} ({})", err, LastErrorStr(err));
        return EngineLaunchResponse::FailedToCreateProcess;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        DWORD err = GetLastError();
        Out("Inject", "Failed to get exit code for injector process: {} ({})", err, LastErrorStr(err));
        return EngineLaunchResponse::Failed;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<EngineLaunchResponse>(exitCode);
#elif defined(__unix__) || defined(__APPLE__)
    // where wine comes in
    pid_t pid = 0;
    std::filesystem::path wine_path = GetUserDataDir() / NW_PATH_WINE;
    std::filesystem::path wine_root = GetUserDataDir() / NW_PATH_WINE_ROOT;
    std::filesystem::path wine_prefix = GetUserDataDir() / NW_PATH_WINE_PREFIX;
    std::filesystem::path wine_exe = wine_root / "bin" / "wine";

    std::string wineprefix_env = "WINEPREFIX=" + wine_prefix.generic_string();

    std::vector<char*> argv_ptrs;
    argv_ptrs.push_back((char*)wine_exe.c_str());
    for (auto& arg : argsStr) {
        argv_ptrs.push_back(arg.data());
    }
    argv_ptrs.push_back(nullptr);

    char* wine_environ[] = {(char*)wineprefix_env.c_str(), NULL};

    // Launch the process
    int status = posix_spawn(&pid, wine_exe.c_str(), NULL, NULL, argv_ptrs.data(), wine_environ);

    if (status == 0) {
        std::cout << "Launched process with PID: " << pid << std::endl;
        // waitpid(pid, &status, 0);
    } else {
        perror("posix_spawn failed");
    }
    return EngineLaunchResponse::Success;
#endif
}

// Notes about getting Roblox working
// FFlagDebugLocalRccServerConnection is required to be set in order to prevent Id 24 error
EngineLaunchResponse Core::LaunchEngine(EngineStartParameters params) {
    if (params.Port.has_value()) {
        if (params.Engine.Side == EngineSide::Client) {
            mServerEmulator->AddTemporaryProxy(params.Ip, *params.Port);
        } else if (params.Engine.Side == EngineSide::Server) {
            mServerEmulator->AddGameServer(params);
        }
    }

    bool installed = IsEngineInManifest(params.Engine);
    if (!installed) return EngineLaunchResponse::NotInstalled;
    const std::filesystem::path dir = GetEngineDirectory(params.Engine);
    std::filesystem::path exe;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(dir)) {
        std::string fn = entry.path().filename().string();
        if (fn == "RobloxPlayerBeta.exe" || fn == "RCCService.exe" || fn == "RobloxStudioBeta.exe") {
            exe = entry.path();
            break;
        }
    }
    if (!exe.empty()) {
        mServerEmulator->SetCurrentEngine(params.Engine);
        return LaunchProcessThroughInjector(params.Engine.Architecture, exe, params);
    } else {
        return EngineLaunchResponse::NoValidExecutable;
    }
}