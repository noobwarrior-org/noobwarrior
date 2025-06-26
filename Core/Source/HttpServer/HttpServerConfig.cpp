// === noobWarrior ===
// File: HttpServerConfig.cpp
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#include <NoobWarrior/HttpServer/HttpServerConfig.h>

using namespace NoobWarrior;

HttpServer::Config::Config(const std::filesystem::path &filePath, lua_State *luaState) : BaseConfig(filePath, luaState),
    Port(8080),
    Password(nullptr)
{}

void HttpServer::Config::OnDeserialize() {
    DeserializeProperty<uint16_t>(&Port, "port");
    DeserializeProperty<char*>(&Password, "password");
}

void HttpServer::Config::OnSerialize() {
}
