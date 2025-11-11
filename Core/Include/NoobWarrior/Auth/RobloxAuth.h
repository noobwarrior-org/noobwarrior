// === noobWarrior ===
// File: RobloxAuth.h
// Started by: Hattozo
// Started on: 11/7/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/BaseAuth.h>

namespace NoobWarrior {
struct RobloxAccount {
    int64_t Id;
    std::string Name;
    std::string Token; // .ROBLOSECURITY token
    std::string Url;
    long ExpireTimestamp { -1 }; // Defaults to -1 if the expiration date is unknown.
};

class RobloxAuth : public BaseAuth<RobloxAccount> {
public:
    RobloxAuth(Config *config);
    std::string GetName() override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}