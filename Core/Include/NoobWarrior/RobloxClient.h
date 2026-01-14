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
// File: RobloxClient.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include <string>

namespace NoobWarrior {
constexpr int ClientTypeCount = 2;
enum class ClientType {
    Client,
    Server,
    Studio
};

inline const char *ClientTypeAsTranslatableString(ClientType type) {
    switch (type) {
    case ClientType::Client: return "Client";
    case ClientType::Server: return "Server";
    case ClientType::Studio: return "Studio";
    }
    return "None";
}

struct RobloxClient {
    int                     NoobWarriorVersion  {};
    ClientType              Type                {};
    std::string             Hash                {};
    std::string             Version             {};
};

enum class ClientInstallState {
    Failed,
    Success,
    RetrievingIndex,
    DownloadingFiles,
    ExtractingFiles
};

enum class ClientLaunchResponse {
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