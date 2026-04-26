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

using namespace NoobWarrior;

std::vector<Engine> Core::GetInstalledEngines() {
    std::vector<Engine> engines;
    if (!std::filesystem::exists(GetUserDataDir() / NW_PATH_ENGINES))
        return engines;

    for (const auto &entry : std::filesystem::recursive_directory_iterator(GetUserDataDir() / NW_PATH_ENGINES)) {
        if (entry.path().extension().compare("exe")) {
            engines.push_back({
                .Source = EngineSource::Local,
                .Platform = EnginePlatform::Windows,
                .FilePath = entry.path()
            });
        }
    }

    return engines;
}

std::vector<Engine> Core::GetEnginesFromIndex() {
    nlohmann::json index;
    int res = RetrieveIndex(index);
    if (res != CURLE_OK)
        return {};
    return {};
}

std::vector<Engine> Core::GetAllEngines() {
    return {};
}

std::filesystem::path Core::GetEngineDirectory(const Engine &engine) {
    std::filesystem::path dir = GetUserDataDir();
    switch (engine.Type) {
    default:
        switch (engine.Side) {
        case EngineSide::Client: dir = dir / NW_PATH_ENGINES_ROBLOX_CLIENT; break;
        case EngineSide::Server: dir = dir / NW_PATH_ENGINES_ROBLOX_SERVER; break;
        case EngineSide::Studio: dir = dir / NW_PATH_ENGINES_ROBLOX_STUDIO; break;
        }
        break;
    }  
    dir /= ("version-" + engine.Hash);
    return dir;
}

void Core::DiscoverEngines() {

}

bool Core::IsEngineInstalled(const Engine &engine) {
    if (!std::filesystem::exists(GetEngineDirectory(engine))) return false;
    bool foundExe = false;
    for (const auto &entry : std::filesystem::directory_iterator(GetEngineDirectory(engine))) {
        if (entry.path().extension() == ".exe")
            foundExe = true;
    }
    return foundExe;
}

void Core::DownloadAndInstallEngine(const Engine &engine, std::shared_ptr<std::vector<std::shared_ptr<Transfer>>> &transfers, std::shared_ptr<std::function<void(EngineInstallState, CURLcode, size_t, size_t)>> callback) {
    nlohmann::json index;
    int res = RetrieveIndex(index);
    if (res != CURLE_OK) {
        Out("Download", "Failed to retrieve index");
        (*callback)(EngineInstallState::Failed, static_cast<CURLcode>(res), 0, 0);
        return;
    }

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
    // std::wstring wargs = std::format(L"{} --file \"{}\" --ip 127.0.0.1 --port 53640 --local %7B%22Id%22%3A5%2C%22Name%22%3A%22Player%22%2C%22DisplayName%22%3A%22Player%22%2C%22HumanoidDescription%22%3A%7B%22HeadColor%22%3A%22Bright+yellow%22%2C%22TorsoColor%22%3A%22Bright+blue%22%2C%22LeftArmColor%22%3A%22Bright+yellow%22%2C%22RightArmColor%22%3A%22Bright+yellow%22%2C%22LeftLegColor%22%3A%22Br.+yellowish+green%22%2C%22RightLegColor%22%3A%22Br.+yellowish+green%22%2C%22GraphicTShirt%22%3A1000%2C%22Shirt%22%3A86121841%2C%22Pants%22%3A86121841%2C%22Face%22%3A1000%2C%22Accessories%22%3A%5B%5D%2C%22Head%22%3A0%2C%22Torso%22%3A0%2C%22LeftArm%22%3A0%2C%22RightArm%22%3A0%2C%22LeftLeg%22%3A0%2C%22RightLeg%22%3A0%2C%22ClimbAnimation%22%3A0%2C%22FallAnimation%22%3A0%2C%22IdleAnimation%22%3A0%2C%22JumpAnimation%22%3A0%2C%22RunAnimation%22%3A0%2C%22SwimAnimation%22%3A0%2C%22WalkAnimation%22%3A0%2C%22BodyTypeScale%22%3A0%2C%22DepthScale%22%3A0%2C%22HeadScale%22%3A0%2C%22HeightScale%22%3A0%2C%22ProportionScale%22%3A0%2C%22WidthScale%22%3A0%7D%7D", injectorPath.wstring(), filePath.wstring());
    std::wstring wargs = std::format(L"{} --file \"{}\"{}{}",
        injectorPath.wstring(),
        filePath.wstring(),
        !params.Ip.empty() ? L" --ip " + ipStrW : L"",
        params.Port.has_value() ? L" --port " + std::to_wstring(params.Port.value()) : L""
    );
    Out("Inject", "Launching process \"{}\"", converter.to_bytes(wargs));
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');
#if defined(_WIN32)
    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
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
    return EngineLaunchResponse::Failed;
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

    bool installed = IsEngineInstalled(params.Engine);
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