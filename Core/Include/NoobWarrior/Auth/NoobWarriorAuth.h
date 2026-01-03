// === noobWarrior ===
// File: NoobWarriorAuth.h
// Started by: Hattozo
// Started on: 11/10/2025
// Description: List of accounts for connecting to servers that use noobWarrior infrastracture
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/BaseAuth.h>

namespace NoobWarrior {
struct Account {
    int64_t Id;
    std::string Name;
    std::string Token;
    std::string Url;
    long ExpireTimestamp { -1 }; // Defaults to -1 if the expiration date is unknown.
};

class NoobWarriorAuth : public BaseAuth<Account> {
public:
    NoobWarriorAuth(Config *config);
    std::string GetName() override = 0;
    nlohmann::json AccStructToJson(Account &acc) override;
    Account AccJsonToStruct(nlohmann::json &json) override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}