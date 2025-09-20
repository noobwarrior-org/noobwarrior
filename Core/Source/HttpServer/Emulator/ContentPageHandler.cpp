// === noobWarrior ===
// File: ContentPageHandler.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/ContentPageHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/ReflectionMetadata.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ContentPageHandler::ContentPageHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json ContentPageHandler::GetContextData() {
    auto data = WebHandler::GetContextData();
    Config *config = mServer->GetCore()->GetConfig();

    data["idtypes"] = nlohmann::json::array();
    Out("ContentPageHandler", "Yahe");
    for (std::pair pair : Reflection::GetIdTypes()) {
        Out("ContentPageHandler", "ID Type {}", pair.first);
        data["idtypes"].push_back(pair.first);
    }

    return data;
}