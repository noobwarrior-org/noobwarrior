// === noobWarrior ===
// File: UserProfileViewHandler.cpp
// Started by: Hattozo
// Started on: 10/4/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/UserProfileViewHandler.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

UserProfileViewHandler::UserProfileViewHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json UserProfileViewHandler::GetContextData(mg_connection *conn) {
    auto data = WebHandler::GetContextData();

    return data;
}