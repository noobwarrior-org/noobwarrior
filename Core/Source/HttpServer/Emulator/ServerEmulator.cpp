// === noobWarrior ===
// File: ServerEmulator.cpp
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include "NoobWarrior/HttpServer/Emulator/LibraryPageHandler.h"
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator", "emulator") {

}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    mAssetHandler = std::make_unique<AssetHandler>(this, mCore->GetDatabaseManager());
    mLibraryPageHandler = std::make_unique<LibraryPageHandler>(this);

    SetRequestHandler("/asset", mAssetHandler.get());
    SetRequestHandler("/v1/asset", mAssetHandler.get());

    SetRequestHandler("/develop", mLibraryPageHandler.get(), (void*)"content.jinja");
    
    NOOBWARRIOR_LINK_URI_TO_TEMPLATE("/login", "login.jinja")
    NOOBWARRIOR_LINK_URI_TO_TEMPLATE("/home", "home.jinja")
finish:
    return res;
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}

nlohmann::json ServerEmulator::GetBaseContextData() {
    auto data = HttpServer::GetBaseContextData();
    Config *config = mCore->GetConfig();

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
