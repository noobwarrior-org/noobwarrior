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
#include <string>

namespace NoobWarrior {
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

inline const char *EngineSideAsTranslatableString(EngineSide side) {
    switch (side) {
    case EngineSide::Client: return "Client";
    case EngineSide::Server: return "Server";
    case EngineSide::Studio: return "Studio";
    }
    return "None";
}

struct Engine {
    EngineType  Type    {};
    EngineSide  Side    {};
    std::string Hash    {};
    std::string Version {};
};

enum class EngineInstallState {
    Failed,
    Success,
    RetrievingIndex,
    DownloadingFiles,
    ExtractingFiles
};

enum class EngineLaunchResponse {
    Failed,
    Success,
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
    InjectFailedToResumeProcess
};
}