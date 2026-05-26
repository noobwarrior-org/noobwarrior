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
// File: Engine.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include <NoobWarrior/Url.h>

#include <string>
#include <ctime>
#include <optional>

namespace NoobWarrior {
enum class EngineSource {
    Local,
    Remote
};

enum class EngineOs {
    Windows,
    Mac,
    Linux,
    Android,
    Ios
};

enum class EngineArchitecture {
    x86,
    x86_64
};

constexpr int EngineTypeCount = 1;
enum class EngineType {
    Roblox
};

constexpr int EngineSideCount = 3;
enum class EngineSide {
    Client,
    Server,
    Studio
};

inline std::string EngineTypeAsString(EngineType side) {
    switch (side) {
    case EngineType::Roblox: return "Roblox";
    }
    return "None";
}

inline std::string EngineSideAsString(EngineSide side) {
    switch (side) {
    case EngineSide::Client: return "Client";
    case EngineSide::Server: return "Server";
    case EngineSide::Studio: return "Studio";
    }
    return "None";
}

struct Engine {
    EngineSource            Source          {};
    EngineOs                Os              {};
    EngineArchitecture      Architecture    {};
    EngineType              Type            {};
    EngineSide              Side            {};
    std::string             Hash            {};
    std::string             Version         {};
    std::filesystem::path   FilePath        {}; // local only
    Url                     RemoteUrl       {}; // remote only
    time_t                  Date            {};
};

struct EngineStartParameters {
    Engine Engine {};
    std::string Ip {};
    std::optional<uint16_t> Port { std::nullopt };
    std::optional<int64_t> PlaceId { std::nullopt };
    std::optional<EngineSide> LaunchSide {};
};

enum class EngineInstallState {
    Failed,
    Success,
    RetrievingIndex,
    DownloadingFiles,
    ExtractingFiles
};

enum class EngineLaunchResponse {
    Success,
    Failed,
    NotInstalled,
    NoValidExecutable,
    FailedToCreateProcess,
    InjectFailed,
    InjectDllMissing,
    InjectCannotAccessProcess,
    InjectWrongArchitecture,
    InjectCannotWriteToProcessMemory,
    InjectFailedToGetModuleHandle,
    InjectFailedToGetFunctionAddress,
    InjectCannotCreateThreadInProcess,
    InjectThreadTimedOut,
    InjectCouldNotGetReturnValueOfLoadLibrary,
    InjectFailedToLoadLibrary,
    InjectFailedToResumeProcess,
    WineMissing
};

std::string GetEngineVersion(const Engine &engine);
}