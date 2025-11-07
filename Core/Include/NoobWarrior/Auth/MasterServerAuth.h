// === noobWarrior ===
// File: MasterServerAuth.h
// Started by: Hattozo
// Started on: 11/7/2025
// Description:
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/BaseAuth.h>

namespace NoobWarrior {
struct MasterServerAccount : public NoobWarrior::Account {
    std::string Url;
};

class MasterServerAuth : public BaseAuth<MasterServerAccount> {
public:
    MasterServerAuth(Config *config);
    std::string GetName() override;
    nlohmann::json AccStructToJson(MasterServerAccount &acc) override;
    MasterServerAccount AccJsonToStruct(nlohmann::json &json) override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}