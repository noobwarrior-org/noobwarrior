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
#include <cpr/cpr.h>

#include <NoobWarrior/Log.h>
#include <NoobWarrior/Engine.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/PeFile.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Roblox/FileFormat/StudioServerPlace.h>
#include <NoobWarrior/Roblox/FileFormat/StudioServerPlaceCache.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AvatarAppearance.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/Paths.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <zip.h>
#include <zipconf.h>

#include <chrono>
#include <thread>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#endif

#include <climits>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <unordered_map>

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__ANDROID__)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

#define NOOBWARRIOR_WINE_VERSION "wine-11.14"

using namespace NoobWarrior;

static void ReportEngineLaunchProgress(const EngineLaunchProgressCallback &callback,
                                       EngineLaunchStage stage, double progress,
                                       uint64_t placeBytes = 0) {
    if (callback)
        callback({stage, progress, placeBytes});
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
    engine.Architecture = Pe::ReadMachine(exe) == Pe::Machine::x86_64 ? EngineArchitecture::x86_64
                                                                      : EngineArchitecture::x86;
    engine.Version      = Pe::ReadProductVersion(exe);
    return engine;
}

std::vector<Engine> Core::GetInstalledEngines() {
    std::vector<Engine> engines;
    std::filesystem::path enginesDir = GetUserDataDir() / NW_PATH_ENGINES;
    if (!std::filesystem::exists(enginesDir))
        return engines;

    // Collect entries sorted by last_write_time ascending so older (earlier-installed)
    // engines are preferred when PickBestMatch falls back to side-only matching.
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto &entry : std::filesystem::directory_iterator(enginesDir)) {
        if (entry.is_directory())
            entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
        return a.last_write_time() < b.last_write_time();
    });

    for (const auto &entry : entries) {
        if (auto engine = InspectEngineDirectory(entry.path())) {
            Out("Engine", "Detected engine in \"{}\": side={} arch={} version=\"{}\"",
                entry.path().filename().string(),
                EngineSideAsString(engine->Side),
                engine->Architecture == EngineArchitecture::x86_64 ? "x86_64" : "x86",
                engine->Version);
            engines.push_back(*engine);
        } else {
            Out("Engine", "Skipping \"{}\" - no recognised Roblox executable found inside",
                entry.path().filename().string());
        }
    }
    return engines;
}

std::vector<Engine> Core::GetAllEngines() {
    return {};
}

int NoobWarrior::ParseEraVersion(const std::string &version) {
    if (version.empty()) return -1;
    size_t firstDot = version.find('.');
    if (firstDot == std::string::npos) return -1;
    size_t secondDot = version.find('.', firstDot + 1);
    std::string era = version.substr(firstDot + 1,
        secondDot == std::string::npos ? std::string::npos : secondDot - firstDot - 1);
    try { return std::stoi(era); } catch (...) { return -1; }
}

static std::optional<Engine> PickBestMatch(const std::vector<Engine> &installed, const Engine &want) {
    const Engine *exactMatch = nullptr;
    const Engine *closestEra = nullptr;
    int           closestDelta = INT_MAX;
    const Engine *sideOnlyFallback = nullptr;

    const int wantEra = ParseEraVersion(want.Version);

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

        const int candidateEra = ParseEraVersion(candidate.Version);
        if (wantEra >= 0 && candidateEra >= 0) {
            int delta = std::abs(wantEra - candidateEra);
            if (delta < closestDelta) {
                closestDelta = delta;
                closestEra = &candidate;
            }
        }

        if (sideOnlyFallback == nullptr)
            sideOnlyFallback = &candidate;
    }

    if (exactMatch) return *exactMatch;
    if (closestEra) return *closestEra;
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

std::optional<Engine> Core::ResolveInstalledEngine(const Engine &want) {
    return PickBestMatch(GetInstalledEngines(), want);
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

// TODO: FINISH THIS SHIT
void Core::UpdateWine(std::function<bool(WineUpdateState, double)> callback) {
    std::filesystem::path wineRoot = GetUserDataDir() / NW_PATH_WINE_ROOT;
    std::filesystem::path wine = wineRoot / "bin" / "wine";
    bool needsUpdating = false;
    if (!std::filesystem::exists(wine)) {
        needsUpdating = true;
    } else {
#if defined(__unix__) || defined(__APPLE__)
        if (fork() == 0) {
            execlp(wine.c_str(), wine.c_str(), "--version", nullptr);
            
        }
#endif
    }

    if (needsUpdating) {
        cpr::Session session {};
        session.SetUrl("https://github.com/Kron4ek/Wine-Builds/releases/download/11.14/wine-11.14-amd64-wow64.tar.xz");

        session.SetOption(cpr::ProgressCallback {
            [callback](cpr::cpr_pf_arg_t downloadTotal, cpr::cpr_pf_arg_t downloadNow, cpr::cpr_pf_arg_t uploadTotal, cpr::cpr_pf_arg_t uploadNow, intptr_t userdata) -> bool {
                callback(WineUpdateState::DownloadingWine, static_cast<double>(downloadNow) / downloadTotal);
                // reminder: returning true will continue the transfer, returning false will abort it
                return true;
            }
        });

        session.GetCallback([](cpr::Response response) {
            // callback()
        });

        std::thread t;
    }
}

void Core::DownloadAndInstallEngine(const Engine &engine, std::function<void()> callback) {
    nlohmann::json index;

    bool foundEngine = false;

    if (!foundEngine) {
        Out("Download", "Failed to find a client");
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

static bool MergeEmulatorCertIntoEngineCaBundle(const std::filesystem::path &emuCertPath,
                                                 const std::filesystem::path &engineDir) {
    static constexpr const char *kMarker = "# noobWarrior emulator CA";
    const std::filesystem::path caBundle = engineDir / "ssl" / "cacert.pem";
    const std::filesystem::path backup = engineDir / "ssl" / "cacert.pem.noobwarrior.bak";

    auto readFile = [](const std::filesystem::path &path, std::string &contents) {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
            return false;
        contents.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        return !input.bad();
    };

    std::string emulatorCert;
    if (!readFile(emuCertPath, emulatorCert) || emulatorCert.empty()) {
        Out("LaunchEngine", "Could not read emulator CA certificate {}", emuCertPath.string());
        return false;
    }

    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(caBundle, filesystemError) || filesystemError) {
        Out("LaunchEngine", "Could not find engine CA bundle {}", caBundle.string());
        return false;
    }

    if (!std::filesystem::exists(backup, filesystemError)) {
        filesystemError.clear();
        if (!std::filesystem::copy_file(caBundle, backup, std::filesystem::copy_options::none,
                                        filesystemError)) {
            // Another simultaneous launch may have created the backup between exists and copy_file.
            std::error_code backupError;
            if (!std::filesystem::is_regular_file(backup, backupError) || backupError) {
                Out("LaunchEngine", "Could not back up engine CA bundle {} to {}: {}",
                    caBundle.string(), backup.string(), filesystemError.message());
                return false;
            }
        } else {
            Out("LaunchEngine", "Backed up engine CA bundle to {}", backup.string());
        }
    } else if (filesystemError || !std::filesystem::is_regular_file(backup, filesystemError) || filesystemError) {
        Out("LaunchEngine", "Engine CA backup is not a readable file: {}", backup.string());
        return false;
    }

    // Always rebuild from the durable backup. If that backup came from an older direct-merge run,
    // remove its tagged block in memory so the current certificate remains idempotent.
    std::string baseBundle;
    if (!readFile(backup, baseBundle)) {
        Out("LaunchEngine", "Could not read engine CA backup {}", backup.string());
        return false;
    }
    if (const size_t markerPosition = baseBundle.find(kMarker); markerPosition != std::string::npos)
        baseBundle.erase(markerPosition);

    std::string merged = baseBundle;
    if (!merged.empty() && merged.back() != '\n')
        merged.push_back('\n');
    merged += kMarker;
    merged.push_back('\n');
    merged += emulatorCert;
    if (!merged.empty() && merged.back() != '\n')
        merged.push_back('\n');

    std::string installedBundle;
    if (readFile(caBundle, installedBundle) && installedBundle == merged) {
        Out("LaunchEngine", "Engine CA bundle already includes the current emulator CA; backup is {}",
            backup.string());
        return true;
    }

    std::ofstream output(caBundle, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        Out("LaunchEngine", "Could not open {} to merge emulator CA", caBundle.string());
        return false;
    }
    output.write(merged.data(), static_cast<std::streamsize>(merged.size()));
    output.close();
    if (!output) {
        Out("LaunchEngine", "Could not finish writing merged engine CA bundle {}; restore from {}",
            caBundle.string(), backup.string());
        return false;
    }

    Out("LaunchEngine", "Merged emulator CA into {}; original backup is {}",
        caBundle.string(), backup.string());
    return true;
}

EngineLaunchResponse Core::LaunchProcessThroughInjector(EngineArchitecture arch, const std::filesystem::path &filePath, EngineStartParameters params) {
    const std::filesystem::path &injectorPath = GetInstallDataDir() / (arch == EngineArchitecture::x86_64 ? "noobhook_x86-64_injector.exe" : "noobhook_x86_injector.exe");
    if (!std::filesystem::exists(injectorPath)) {
        Out("Inject", "Failed to create injector process: Injector process doesn't exist!");
        return EngineLaunchResponse::FailedToCreateProcess;
    }

    std::vector<std::string> args;
    args.push_back(injectorPath.string());
    args.push_back("--file");
#if defined(_WIN32)
    args.push_back("\"" + filePath.string() + "\"");
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

    // Auth-mode launch ticket the client replays to PlaceLauncher (see LaunchEngine). Substituted for
    // the injector's default -t "1".
    if (params.LaunchTicket.has_value() && !params.LaunchTicket->empty()) {
        args.push_back("--authticket");
        args.push_back("\"" + *params.LaunchTicket + "\"");
    }

    EngineSide launchSide = params.LaunchSide.value_or(params.Engine.Side);
    const char* sideStr = launchSide == EngineSide::Client ? "client"
                        : launchSide == EngineSide::Server ? "server"
                        : "studio";
    args.push_back("--side");
    args.push_back(sideStr);

    uint16_t emuHttpPort  = static_cast<uint16_t>(mRegistry->GetKeyValue<int>("emu.http_port").value_or(8080));
    uint16_t emuHttpsPort = static_cast<uint16_t>(mRegistry->GetKeyValue<int>("emu.https_port").value_or(53640));
    args.push_back("--emuhttp");
    args.push_back(std::to_string(emuHttpPort));
    args.push_back("--emuhttps");
    args.push_back(std::to_string(emuHttpsPort));

    // Select arguments from the executable that will actually be launched. params.Engine may be a
    // requested version which PickBestMatch resolved to a different installed build.
    const std::string detectedVersion = Pe::ReadProductVersion(filePath);
    const int era = ParseEraVersion(detectedVersion.empty() ? params.Engine.Version : detectedVersion);

    // 0.574+'s Player and Studio HTTP stacks explicitly open ssl/cacert.pem beside the executable.
    // Preserve the original bundle once, then install an idempotent merge in that existing location.
    std::filesystem::path emuCert = GetUserDataDir() / NW_PATH_SSL / "cert.pem";
    const bool usesMergedEngineCa = era >= 574 &&
        (filePath.filename() == "RobloxPlayerBeta.exe" ||
         filePath.filename() == "RobloxStudioBeta.exe");
    if (usesMergedEngineCa && !MergeEmulatorCertIntoEngineCaBundle(emuCert, filePath.parent_path()))
        return EngineLaunchResponse::Failed;

    // Opening Player from the launcher and joining a server are distinct operations.
    // Player 0.719 needs a dedicated direct-join argument set in the injector;
    // launching without a join target intentionally opens the ordinary app UI.
    // Keep the established --play path for 0.574 and the other earlier clients.
    const bool hasJoinTarget = !params.Ip.empty() || params.Port.has_value() ||
                               params.PlaceId.has_value();
    args.push_back("--scheme");
    if (era == 719)
        args.push_back(hasJoinTarget ? "app" : "home");
    else
        args.push_back(era == 463 ? "old" : "new");

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
    if (!CreateProcessA(nullptr, const_cast<char*>(argsStr.c_str()), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        Out("Inject", "Failed to create injector process: {} ({})", err, LastErrorStr(err));
        return EngineLaunchResponse::FailedToCreateProcess;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return EngineLaunchResponse::Success;
#elif (defined(__unix__) || defined(__APPLE__)) && !defined(__ANDROID__)
    pid_t pid = 0;
    std::filesystem::path wine_root   = GetUserDataDir() / NW_PATH_WINE_ROOT;
    std::filesystem::path wine_prefix_dir = GetUserDataDir() / NW_PATH_WINE_PREFIX;
    std::filesystem::path bundled_wine = wine_root / "bin" / "wine";

    // Use bundled wine if it exists, or use system wine (assuming they installed it through their pkg manager)
    std::string wine_exe_str;
    bool wine_is_absolute;
    if (std::filesystem::exists(bundled_wine)) {
        wine_exe_str = bundled_wine.string();
        wine_is_absolute = true;
    } else {
        wine_exe_str = mRegistry->GetKeyValue<std::string>("wine.exe").value_or("wine");
        wine_is_absolute = std::filesystem::path(wine_exe_str).is_absolute();
    }
    if (wine_exe_str.empty()) {
        return EngineLaunchResponse::WineMissing;
    }
    
    std::string winePrefix = mRegistry->GetKeyValue<std::string>("wine.prefix").value_or("");
    if (winePrefix.empty())
        winePrefix = wine_prefix_dir.generic_string();
    std::string wineprefix_env = "WINEPREFIX=" + winePrefix;

    std::vector<char*> argv_ptrs;
    argv_ptrs.push_back(wine_exe_str.data());
    for (auto& arg : args)
        argv_ptrs.push_back(arg.data());
    argv_ptrs.push_back(nullptr);

    std::vector<std::string> env_strings;
    for (char** ep = environ; ep && *ep; ep++) {
        if (strncmp(*ep, "WINEPREFIX=", 11) != 0) {
            env_strings.push_back(*ep);
        }
    }
    env_strings.push_back(wineprefix_env);
    std::vector<char*> env_ptrs;
    for (auto& s : env_strings)
        env_ptrs.push_back(s.data());
    env_ptrs.push_back(nullptr);

    Out("Inject", "Launching via Wine ({}): {}", wine_exe_str, argsStr);
    int status;
    if (wine_is_absolute)
        status = posix_spawn(&pid, wine_exe_str.c_str(), nullptr, nullptr, argv_ptrs.data(), env_ptrs.data());
    else
        status = posix_spawnp(&pid, wine_exe_str.c_str(), nullptr, nullptr, argv_ptrs.data(), env_ptrs.data());

    if (status != 0) {
        Out("Inject", "posix_spawn failed: {}", strerror(status));
        return EngineLaunchResponse::FailedToCreateProcess;
    }
    Out("Inject", "Launched Wine process with PID {}", pid);
    return EngineLaunchResponse::Success;
#else
    // Android and other platforms: PE engines aren't launchable here.
    return EngineLaunchResponse::FailedToCreateProcess;
#endif
}

bool Core::WriteGameServerConfig(const std::filesystem::path &engineDir, const EngineStartParameters &params) {
    int64_t placeId = params.PlaceId.value_or(0);
    int64_t universeId = mEmuDbManager.GetUniverseIdForPlace(placeId).value_or(placeId);
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
        {"UniverseId", universeId},
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

bool Core::WriteServerRbxl(int64_t placeId, int version,
                           const StudioServerBootstrap& serverBootstrap,
                           const EngineLaunchProgressCallback& progressCallback,
                           std::string_view studioFingerprint) {
#if defined(_WIN32)
    WCHAR *path;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &path);
    std::filesystem::path baseDir(path);
    CoTaskMemFree(path);
#else
    // placeholder so that it doesnt fail to compile
    std::filesystem::path baseDir(std::filesystem::current_path());
    return false;
#endif
    std::filesystem::path serverRbxlPath = baseDir / "Roblox" / "server.rbxl";
    ReportEngineLaunchProgress(progressCallback,
                               EngineLaunchStage::LoadingStudioServerPlace, 0.15);
    EmuDb* db = mEmuDbManager.GetFirstDbWhereItemExists(ItemType::Asset, placeId);
    if (db == nullptr) {
        Out("Core", "Could not find a database containing place ID {}", placeId);
        return false;
    }

    if (mRegistry->GetKeyValue<bool>("disable_rbxl_mutation") == true) {
        std::vector<unsigned char> data;
        SqlDb::Response res = db->RetrieveAssetData(placeId, version, &data);
        if (res != SqlDb::Response::Success || data.empty()) {
            Out("Core", "Failed to read data from place ID {}", placeId);
            return false;
        }

        std::ofstream stream(serverRbxlPath, std::ios::binary);
        if (!stream.is_open()) {
            Out("Core", "Failed to open std::ofstream object for \"{}\"", serverRbxlPath.string());
            return false;
        }
        stream.write(reinterpret_cast<const char*>(data.data()), data.size());
        stream.close();
        ReportEngineLaunchProgress(progressCallback, EngineLaunchStage::StartingEngine, 1.0, data.size());
        return true;
    }
    std::string sourceHash;
    const SqlDb::Response hashResponse =
        db->RetrieveAssetDataHash(placeId, version, &sourceHash);
    if (hashResponse != SqlDb::Response::Success || sourceHash.empty()) {
        Out("Core", "Failed to read data from place ID {}", placeId);
        return false;
    }

    const std::filesystem::path cacheDirectory =
        GetUserDataDir() / NW_PATH_TEMP / "studio-server-places" /
        ("v" + std::to_string(kStudioServerPlaceCacheSchema));
    uint64_t placeBytes = 0;
    std::string error;
    const StudioServerPlacePreparationResponse preparation = PrepareStudioServerPlace(
        sourceHash, serverBootstrap, studioFingerprint, cacheDirectory, serverRbxlPath,
        [&](std::vector<unsigned char> &data, std::string *loadError) {
            const SqlDb::Response response =
                db->RetrieveAssetData(placeId, version, &data);
            if (response != SqlDb::Response::Success || data.empty()) {
                if (loadError != nullptr)
                    *loadError = "could not retrieve place data from the database";
                return false;
            }

            if (!serverBootstrap.Empty()) {
                ReportEngineLaunchProgress(progressCallback,
                    EngineLaunchStage::MutatingStudioServerPlace, 0.30, data.size());
            }
            return true;
        }, &placeBytes, &error,
        [&](uint64_t bytes) {
            ReportEngineLaunchProgress(progressCallback,
                EngineLaunchStage::WritingStudioServerPlace, 0.90, bytes);
        });
    if (preparation == StudioServerPlacePreparationResponse::Failed) {
        Out("LaunchEngine", "Could not prepare Studio's launch-only server place: {}", error);
        return false;
    }

    if (preparation == StudioServerPlacePreparationResponse::CacheHit) {
        Out("LaunchEngine", "Using cached {}-byte launch-only server.rbxl", placeBytes);
    } else if (!serverBootstrap.Empty()) {
        size_t modelBytes = 0;
        for (const StudioServerModel &model : serverBootstrap.Models)
            modelBytes += model.Data.size();
        Out("LaunchEngine", "Mounted {} plugin plans, {} launch-scoped scripts, and "
            "{} embedded binary models ({} bytes) into launch-only server.rbxl",
            serverBootstrap.Plans.size(),
            serverBootstrap.Scripts.size(), serverBootstrap.Models.size(), modelBytes);
    }

    ReportEngineLaunchProgress(progressCallback, EngineLaunchStage::StartingEngine, 1.0,
                                placeBytes);
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
// Edit 8/23/26: Thats such an old note!!!!!!!!!!!!
EngineLaunchResponse Core::LaunchEngine(
    EngineStartParameters params,
    const EngineLaunchProgressCallback& progressCallback) {
    if (mServerEmulator != nullptr) {
        mServerEmulator->ClearProxyLayers();
        if (params.RemoteEmulatorHost.has_value() && params.RemoteEmulatorPort.has_value()) {
            mServerEmulator->PushProxyLayer(*params.RemoteEmulatorHost, *params.RemoteEmulatorPort,
                                            params.RemoteEmulatorSessionToken.value_or(""));
            mServerEmulator->SetJoinedPlaceId(params.PlaceId);

            // Federate this client's avatar to the host so its game server builds our character with
            // our own look (the host otherwise only knows its own local appearance). Build the JSON
            // here on the owning thread (registry/Lua aren't thread-safe), then POST it in the
            // background so we don't block the launch on a network round-trip.
            int64_t userId = GetRegistry()->GetKeyValue<int64_t>("user.id").value_or(1000);
            nlohmann::json payload;
            payload["userId"] = userId;
            payload["avatarFetch"] = AvatarAppearance::BuildAvatarFetchJson(this);

            std::thread([host = *params.RemoteEmulatorHost, port = *params.RemoteEmulatorPort,
                         body = payload.dump()]() {
                cpr::Response r = cpr::Post(
                    cpr::Url{"https://" + host + ":" + std::to_string(port) + "/emu/v1/avatar-override"},
                    cpr::Header{{"Content-Type", "application/json"}},
                    cpr::Body{body},
                    cpr::Timeout{std::chrono::seconds(10)},
                    cpr::VerifySsl{false}); // remote emulators use self-signed certs
                if (r.error.code != cpr::ErrorCode::OK)
                    Out("LaunchEngine", "Avatar federation to {}:{} failed: {}", host, port, r.error.message);
                else if (r.status_code >= 400)
                    Out("LaunchEngine", "Avatar federation to {}:{} got HTTP {}", host, port, (long)r.status_code);
            }).detach();
        }
    }

    if (!IsEngineInManifest(params.Engine))
        return EngineLaunchResponse::NotInstalled;

    const std::filesystem::path engineDir = GetEngineDirectory(params.Engine);

    std::filesystem::path exe = FindEngineExecutable(engineDir);
    if (exe.empty()) {
        return EngineLaunchResponse::NoValidExecutable;
    }

    if (params.Engine.Side == EngineSide::Server) {
        if (!WriteGameServerConfig(engineDir, params))
            return EngineLaunchResponse::Failed;
    }

    if (params.Engine.Side == EngineSide::Studio && params.LaunchSide == EngineSide::Server && params.PlaceId.has_value()) {
        int64_t placeId = *params.PlaceId;
        int64_t universeId = mEmuDbManager.GetUniverseIdForPlace(placeId).value_or(placeId);
        ReportEngineLaunchProgress(progressCallback,
                                   EngineLaunchStage::PreparingStudioServerPlace, 0.05);
        StudioServerBootstrap bootstrap =
            mPluginManager.BuildStudioServerBootstrap(placeId, universeId);
        // Studio's StartServer task reads this launch-only place copy. Embedding the
        // bootstrap here keeps scripts out of the separate edit-mode DataModel.
        std::string productVersion = Pe::ReadProductVersion(exe);
        if (productVersion.empty())
            productVersion = params.Engine.Version;
        const std::string studioFingerprint = engineDir.filename().string() + "|" +
            productVersion;
        if (!WriteServerRbxl(placeId, 0, bootstrap, progressCallback, studioFingerprint))
            return EngineLaunchResponse::FailedToLoadPlace;
    }

    if (params.Engine.Side == EngineSide::Studio && mServerEmulator != nullptr) {
        std::string ver  = params.Engine.Version.empty() ? Pe::ReadProductVersion(exe) : params.Engine.Version;
        std::string hash = params.Engine.Hash.empty() ? engineDir.filename().string() : params.Engine.Hash;
        mServerEmulator->SetLaunchedStudioVersion(ver, hash);
        Out("LaunchEngine", "Studio client-version pinned to \"{}\" (upload {})", ver, hash);
    }

    // A launched client has no cookie, so in auth mode mint a launch ticket for the local user and
    // pass it to the injector (-t) for PlaceLauncher to redeem. Remote joins are the host's job.
    {
        EngineSide side = params.LaunchSide.value_or(params.Engine.Side);
        bool authEnabled = GetRegistry() != nullptr &&
                           GetRegistry()->GetKeyValue<bool>("emu.auth.enabled").value_or(false);
        if (authEnabled && side == EngineSide::Client && !params.RemoteEmulatorHost.has_value()) {
            EmuDb *master = GetEmuDbManager() ? GetEmuDbManager()->GetMasterDatabase() : nullptr;
            bool haveToken = params.SessionToken.has_value() && !params.SessionToken->empty();
            if (haveToken && params.SessionToken->rfind("fedvoucher.", 0) == 0) {
                // Federated (slave) join to a server on this same machine (loopback): the voucher can't
                // resolve as a local session, so forward it as the launch ticket - the server redeems it
                // federated-side (AuthTicketRedeemHandler) and stamps the real identity.
                params.LaunchTicket = *params.SessionToken;
                Out("LaunchEngine", "Loopback federated join; forwarding voucher as the launch ticket");
            } else {
                // Identity is the account the user logged in as; with no login the client launches as a
                // guest (the local registry identity is not used in auth mode).
                std::optional<AuthUtil::SessionUser> account;
                if (haveToken)
                    account = AuthUtil::ResolveSessionUser(master, *params.SessionToken);

                if (account) {
                    std::string ticket = AuthUtil::MintAuthTicket(master, account->id, params.PlaceId.value_or(0));
                    if (!ticket.empty()) {
                        params.LaunchTicket = ticket;
                        Out("LaunchEngine", "Minted launch ticket for {} (id {})", account->name, account->id);
                    }
                } else {
                    params.LaunchTicket = AuthUtil::EncodeGuestTicket(AuthUtil::MakeGuestUser());
                    Out("LaunchEngine", "No login; launching as guest");
                }
            }
        }
    }

    return LaunchProcessThroughInjector(params.Engine.Architecture, exe, params);
}
