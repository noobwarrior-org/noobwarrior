// === noobWarrior ===
// File: MasterServerAuth.cpp
// Started by: Hattozo
// Started on: 11/7/2025
// Description:
#include <NoobWarrior/Auth/MasterServerAuth.h>

using namespace NoobWarrior;

MasterServerAuth::MasterServerAuth(Config *config) : BaseAuth(config) {}

std::string MasterServerAuth::GetName() {
    return "master_server";
}

nlohmann::json MasterServerAuth::AccStructToJson(MasterServerAccount &acc) {
    nlohmann::json accJson = BaseAuth::AccStructToJson(acc);
    accJson["url"] = acc.Url;
    return accJson;
}

MasterServerAccount MasterServerAuth::AccJsonToStruct(nlohmann::json &json) {
    MasterServerAccount acc = BaseAuth::AccJsonToStruct(json);
    acc.Url = json["url"];
    return acc;
}
