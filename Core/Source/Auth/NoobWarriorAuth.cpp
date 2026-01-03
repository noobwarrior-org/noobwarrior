// === noobWarrior ===
// File: NoobWarriorAuth.cpp
// Started by: Hattozo
// Started on: 11/10/2025
// Description: List of accounts for connecting to servers that use noobWarrior infrastracture
#include <NoobWarrior/Auth/NoobWarriorAuth.h>

using namespace NoobWarrior;

NoobWarriorAuth::NoobWarriorAuth(Config *config) : BaseAuth(config) {}

nlohmann::json NoobWarriorAuth::AccStructToJson(Account &acc) {
    nlohmann::json accJson = BaseAuth::AccStructToJson(acc);
    accJson["url"] = acc.Url;
    return accJson;
}

Account NoobWarriorAuth::AccJsonToStruct(nlohmann::json &json) {
    Account acc = BaseAuth::AccJsonToStruct(json);
    acc.Url = json["url"];
    return acc;
}
