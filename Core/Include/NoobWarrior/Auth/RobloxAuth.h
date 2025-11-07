// === noobWarrior ===
// File: RobloxAuth.h
// Started by: Hattozo
// Started on: 11/7/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/BaseAuth.h>

namespace NoobWarrior {
class RobloxAuth : public BaseAuth<Account> {
public:
    RobloxAuth(Config *config);
    std::string GetName() override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}