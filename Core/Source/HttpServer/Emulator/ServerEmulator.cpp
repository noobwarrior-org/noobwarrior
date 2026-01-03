// === noobWarrior ===
// File: ServerEmulator.cpp
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;
using json = nlohmann::json;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator", "emulator") {

}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    mAssetHandler = std::make_unique<AssetHandler>(this, mCore->GetDatabaseManager());

    SetRequestHandler("/asset", mAssetHandler.get());
    SetRequestHandler("/v1/asset", mAssetHandler.get());
    
    SetRequestHandler("/v1/settings/application", mClientSettingsHandler.get());
finish:
    return res;
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}

nlohmann::json ServerEmulator::GetBaseContextData(evhttp_request *req) {
    auto data = HttpServer::GetBaseContextData(req);
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

    json home_button = {};
    home_button["name"] = "Home";
    home_button["uri"] = "/home";

    json content_button = {};
    content_button["name"] = "Content";
    content_button["uri"] = "/develop";

    json controlpanel_button = {};
    controlpanel_button["name"] = "Control Panel";
    controlpanel_button["uri"] = "/control-panel";

    json account_button = {};
    account_button["name"] = "Log In";
    account_button["uri"] = "/login";

    /*
    if (conn != nullptr) {
        const mg_request_info *request_info = mg_get_request_info(conn);
        const char* cookie_header = mg_get_header(conn, "Cookie");
        char session_token[1024];
        if (cookie_header) {
            int res = mg_get_cookie(cookie_header, ".LOGINSESSION", session_token, sizeof(session_token));
            if (res > 0) {
                User user;
                bool success = mCore->GetDatabaseManager()->GetUserFromToken(&user, session_token);

                if (success) {
                    account_button["name"] = user.Name;
                    account_button["uri"] = "/users/" + std::to_string(user.Id) + "/profile";
                }
            }
        }
    }
    */

    data["buttons"] = json::array();
    data["buttons"].push_back(home_button);
    data["buttons"].push_back(content_button);
    data["buttons"].push_back(controlpanel_button);
    data["buttons"].push_back(account_button);

    return data;
}
