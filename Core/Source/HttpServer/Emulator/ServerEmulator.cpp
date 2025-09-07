// === noobWarrior ===
// File: ServerEmulator.cpp
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator") {

}

ServerEmulator::~ServerEmulator() {}

int ServerEmulator::Start(uint16_t port) {
    int res = HttpServer::Start(port);
    if (!res) goto finish;

    mAssetHandler = new AssetHandler(this, mCore->GetDatabaseManager());

    SetRequestHandler("/asset", mAssetHandler);
    SetRequestHandler("/v1/asset", mAssetHandler);

    // LINK_URI_TO_TEMPLATE("/", "guest.jinja")
    NOOBWARRIOR_LINK_URI_TO_TEMPLATE("/login", "login.jinja")
    NOOBWARRIOR_LINK_URI_TO_TEMPLATE("/home", "home.jinja")
finish:
    return res;
}

int ServerEmulator::Stop() {
    NOOBWARRIOR_FREE_PTR(mAssetHandler)
    return HttpServer::Stop();
}