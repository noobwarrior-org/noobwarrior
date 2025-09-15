// === noobWarrior ===
// File: EmulatorWebHandler.cpp
// Started by: Hattozo
// Started on: 9/7/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/EmulatorWebHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

EmulatorWebHandler::EmulatorWebHandler(ServerEmulator *server) : WebHandler(server) {
    
}

nlohmann::json EmulatorWebHandler::GetContextData() {
    auto data = WebHandler::GetContextData();
    Config *config = mServer->mCore->GetConfig();

    std::optional game_view_mode = config->GetKeyValue<std::string>("httpserver.game_view_mode");
    std::optional permissions_access_home = config->GetKeyValue<std::string>("httpserver.permissions.access.home");
    std::optional permissions_access_library = config->GetKeyValue<std::string>("httpserver.permissions.access.library");
    std::optional permissions_access_control_panel = config->GetKeyValue<std::string>("httpserver.permissions.access.control_panel");

    data["permissions"]["access"]["home"] = permissions_access_home.has_value() ? permissions_access_home.value() : "user";
    data["permissions"]["access"]["library"] = permissions_access_library.has_value() ? permissions_access_library.value() : "user";
    data["permissions"]["access"]["control_panel"] = permissions_access_control_panel.has_value() ? permissions_access_control_panel.value() : "admin";
    data["game_view_mode"] = game_view_mode.has_value() ? game_view_mode.value() : "server";

    data["servers"] = {
        {
            {"name", "My Server"}
        }
    };

    return data;
}