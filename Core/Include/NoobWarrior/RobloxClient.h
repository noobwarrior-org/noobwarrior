// === noobWarrior ===
// File: RobloxClient.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
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
    ClientType              Type    {};
    std::string             Hash    {};
    std::string             Version {};
};
}