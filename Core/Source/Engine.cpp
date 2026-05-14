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

#include <cstring>
#include <optional>
#include <unordered_map>

#if defined(__unix__) || defined(__APPLE__)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

using namespace NoobWarrior;

static EngineArchitecture ReadPeArchitecture(const std::filesystem::path &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return EngineArchitecture::x86;

    uint8_t dosHeader[64];
    f.read(reinterpret_cast<char*>(dosHeader), sizeof(dosHeader));
    if (f.gcount() < (std::streamsize)sizeof(dosHeader)) return EngineArchitecture::x86;
    if (dosHeader[0] != 'M' || dosHeader[1] != 'Z')      return EngineArchitecture::x86;

    uint32_t peOffset = 0;
    std::memcpy(&peOffset, dosHeader + 0x3C, sizeof(peOffset));

    f.seekg(peOffset, std::ios::beg);
    char sig[4];
    f.read(sig, 4);
    if (f.gcount() < 4)                          return EngineArchitecture::x86;
    if (std::memcmp(sig, "PE\0\0", 4) != 0)      return EngineArchitecture::x86;

    uint16_t machine = 0;
    f.read(reinterpret_cast<char*>(&machine), sizeof(machine));
    return machine == 0x8664 ? EngineArchitecture::x86_64 : EngineArchitecture::x86;
}

static std::string ReadPeProductVersion(const std::filesystem::path &path) {
#if defined(_WIN32)
    std::string pathStr = path.string();
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeA(pathStr.c_str(), &handle);
    if (size == 0) return "";

    std::vector<char> buffer(size);
    if (!GetFileVersionInfoA(pathStr.c_str(), handle, size, buffer.data())) return "";
    
    const char* paths[] = {
        "\\StringFileInfo\\040904E4\\ProductVersion",
        "\\StringFileInfo\\000004B0\\ProductVersion",
    };
    for (const char* p : paths) {
        void *value = nullptr;
        UINT len = 0;
        if (!VerQueryValueA(buffer.data(), p, &value, &len) || value == nullptr)
            continue;
        std::string raw(static_cast<const char*>(value), len);
        while (!raw.empty() && raw.back() == '\0') raw.pop_back();

        // "0, 463, 0, 417004" -> "0.463.0.417004"
        std::string out;
        out.reserve(raw.size());
        for (char c : raw) {
            if (c == ' ') continue;
            out += (c == ',') ? '.' : c;
        }
        return out;
    }
    return "";
#else
    (void)path;
    return "";
#endif
}

static std::optional<Engine> InspectEngineDirectory(const std::filesystem::path &dir) {
    static const std::unordered_map<std::string, EngineSide> exeToSide = {
        {"RobloxPlayerBeta.exe", EngineSide::Client},
        {"RCCService.exe",       EngineSide::Server},
        {"RobloxStudioBeta.exe", EngineSide::Studio},
    };

    std::filesystem::path exe;
    EngineSide side {};
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        auto it = exeToSide.find(entry.path().filename().string());
        if (it != exeToSide.end()) {
            exe = entry.path();
            side = it->second;
            break;
        }
    }
    if (exe.empty()) return std::nullopt;

    Engine engine {};
    engine.Source       = EngineSource::Local;
    engine.Os           = EngineOs::Windows;
    engine.Type         = EngineType::Roblox;
    engine.Side         = side;
    engine.Hash         = dir.filename().string();
    engine.FilePath     = exe;
    engine.Architecture = ReadPeArchitecture(exe);
    engine.Version      = ReadPeProductVersion(exe);
    return engine;
}

std::vector<Engine> Core::GetInstalledEngines() {
    std::vector<Engine> engines;
    std::filesystem::path enginesDir = GetUserDataDir() / NW_PATH_ENGINES;
    if (!std::filesystem::exists(enginesDir))
        return engines;

    for (const auto &entry : std::filesystem::directory_iterator(enginesDir)) {
        if (!entry.is_directory()) continue;
        if (auto engine = InspectEngineDirectory(entry.path())) {
            Out("Engine", "Detected engine in \"{}\": side={} arch={} version=\"{}\"",
                entry.path().filename().string(),
                EngineSideAsString(engine->Side),
                engine->Architecture == EngineArchitecture::x86_64 ? "x86_64" : "x86",
                engine->Version);
            engines.push_back(*engine);
        } else {
            Out("Engine", "Skipping \"{}\" — no recognised Roblox executable found inside",
                entry.path().filename().string());
        }
    }
    return engines;
}

std::vector<Engine> Core::GetAllEngines() {
    return {};
}

static std::optional<Engine> PickBestMatch(const std::vector<Engine> &installed, const Engine &want) {
    const Engine *exactMatch = nullptr;
    const Engine *sideOnlyFallback = nullptr;

    for (const auto &candidate : installed) {
        if (!want.Hash.empty() && candidate.Hash != want.Hash)
            continue;
        if (candidate.Side != want.Side)
            continue;

        if (!want.Version.empty() && !candidate.Version.empty()
                && candidate.Version == want.Version) {
            exactMatch = &candidate;
            break;
        }
        if (sideOnlyFallback == nullptr)
            sideOnlyFallback = &candidate;
    }

    if (exactMatch) return *exactMatch;
    if (sideOnlyFallback) return *sideOnlyFallback;
    return std::nullopt;
}

std::filesystem::path Core::GetEngineDirectory(const Engine &engine) {
    if (!engine.Hash.empty()) {
        std::filesystem::path candidate = GetUserDataDir() / NW_PATH_ENGINES / engine.Hash;
        if (std::filesystem::exists(candidate))
            return candidate;
    }
    if (auto picked = PickBestMatch(GetInstalledEngines(), engine))
        return GetUserDataDir() / NW_PATH_ENGINES / picked->Hash;
    return {};
}

void Core::DiscoverEngines() {

}

bool Core::IsEngineInManifest(const Engine &engine) {
    auto installed = GetInstalledEngines();
    bool ok = PickBestMatch(installed, engine).has_value();
    if (!ok) {
        Out("Engine", "No installed engine matches side={} version=\"{}\" hash=\"{}\" "
                     "(scanned {} engine directories)",
            EngineSideAsString(engine.Side), engine.Version, engine.Hash, installed.size());
    }
    return ok;
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
    if (params.PlaceId.has_value()) {
        args.push_back("--placeid");
        args.push_back(std::to_string(params.PlaceId.value()));
    }

    std::string argsStr;
    for (int i = 0; i < args.size(); i++) {
        argsStr += args[i];
        if (i < args.size() - 1) {
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

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return EngineLaunchResponse::Success;
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
    for (auto& arg : args) {
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

bool Core::WriteGameServerConfig(const std::filesystem::path &engineDir, const EngineStartParameters &params) {
    int64_t placeId = params.PlaceId.value_or(0);
    uint16_t port = params.Port.value_or(53640);

    nlohmann::json settings = {
        {"Type", "Avatar"},
        {"PlaceId", placeId},
        {"CreatorId", 1},
        {"CreatorType", "User"},
        {"GameId", "1"},
        {"MachineAddress", "http://127.0.0.1"},
        {"GsmInterval", 5},
        {"MaxPlayers", 50},
        {"MaxGameInstances", 51},
        {"ApiKey", ""},
        {"PreferredPlayerCapacity", 50},
        {"DataCenterId", "0"},
        {"PlaceVisitAccessKey", ""},
        {"UniverseId", placeId},
        {"PlaceFetchUrl", "http://www.roblox.com/asset/?id=" + std::to_string(placeId)},
        {"MatchmakingContextId", 1},
        {"PlaceVersion", 1},
        {"BaseUrl", "http://www.roblox.com"},
        {"JobId", "Test"},
        {"script", "print('Initializing NetworkServer.')"},
        {"PreferredPort", port},
    };

    nlohmann::json gameServer = {
        {"Mode", "GameServer"},
        {"GameId", placeId},
        {"Arguments", nlohmann::json::object()},
        {"Settings", settings},
    };

    std::ofstream stream(engineDir / "gameserver.json");
    if (!stream.is_open()) {
        Out("LaunchEngine", "Failed to open gameserver.json for writing in {}", engineDir.string());
        return false;
    }
    stream << gameServer << std::endl;
    return true;
}

std::filesystem::path Core::FindEngineExecutable(const std::filesystem::path &engineDir) {
    static const std::set<std::string> knownExes = {
        "RobloxPlayerBeta.exe",
        "RCCService.exe",
        "RobloxStudioBeta.exe",
    };
    for (const auto &entry : std::filesystem::directory_iterator(engineDir)) {
        if (knownExes.contains(entry.path().filename().string()))
            return entry.path();
    }
    return {};
}

// Notes about getting Roblox working
// FFlagDebugLocalRccServerConnection is required to be set in order to prevent Id 24 error
//
// Lifecycle of a launched process is *not* tracked here — noobHook POSTs to
// /v1/process-ping on attach (Hello) and detach (Goodbye), and the ServerEmulator
// maintains its running-instance list off those events. So this function only has
// to set up the config, spawn the injector, and return.
EngineLaunchResponse Core::LaunchEngine(EngineStartParameters params) {
    if (!IsEngineInManifest(params.Engine))
        return EngineLaunchResponse::NotInstalled;

    const std::filesystem::path engineDir = GetEngineDirectory(params.Engine);

    if (params.Engine.Side == EngineSide::Server) {
        if (!WriteGameServerConfig(engineDir, params))
            return EngineLaunchResponse::Failed;
    }

    std::filesystem::path exe = FindEngineExecutable(engineDir);
    if (exe.empty())
        return EngineLaunchResponse::NoValidExecutable;
    
    mServerEmulator->SetCurrentEngine(params.Engine);
    return LaunchProcessThroughInjector(params.Engine.Architecture, exe, params);
}