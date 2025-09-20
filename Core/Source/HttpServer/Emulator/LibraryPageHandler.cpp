// === noobWarrior ===
// File: LibraryPageHandler.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/LibraryPageHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/ReflectionMetadata.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

LibraryPageHandler::LibraryPageHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json LibraryPageHandler::GetContextData() {
    auto data = WebHandler::GetContextData();
    Config *config = mServer->GetCore()->GetConfig();

    data["idtypes"] = nlohmann::json::array();
    for (std::pair pair : Reflection::GetReflectedIdTypes()) {
        data["idtypes"].push_back(pair.first);
    }

    return data;
}