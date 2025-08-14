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
    InjectCannotCreateThreadInProcess,
    InjectCouldNotGetReturnValueOfLoadLibrary,
    InjectFailedToLoadLibrary,
    InjectFailedToResumeProcess
};
}