// === noobWarrior ===
// File: AssetViewHandler.cpp
// Started by: Hattozo
// Started on: 10/4/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/AssetViewHandler.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

AssetViewHandler::AssetViewHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json AssetViewHandler::GetContextData(mg_connection *conn) {
    auto data = WebHandler::GetContextData();

    return data;
}