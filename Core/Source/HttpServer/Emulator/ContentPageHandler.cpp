// === noobWarrior ===
// File: ContentPageHandler.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/ContentPageHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Database/Item/Asset.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

#define ADD_ITEMTYPE(type) data["itemtypes"].push_back(#type);

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ContentPageHandler::ContentPageHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json ContentPageHandler::GetContextData(mg_connection *conn) {
    auto data = WebHandler::GetContextData();
    Config *config = mServer->GetCore()->GetConfig();

    data["itemtypes"] = nlohmann::json::array();
    
    ADD_ITEMTYPE(Asset)

    data["currentpage"] = 1;
    data["totalpages"] = 1;
    data["itemtype"] = "Asset";

    return data;
}