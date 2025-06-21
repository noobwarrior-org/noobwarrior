// === noobWarrior ===
// File: NoobWarrior.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#include <NoobWarrior/NoobWarrior.h>
#include <civetweb.h>

using namespace NoobWarrior;

Core::Core(Init init) :
    mConfig(init.Portable, init.ConfigFileName),
    mHttpServer(nullptr)
{
    mg_init_library(0);
}

Config *Core::GetConfig() {
    return &mConfig;
}

int Core::StartHttpServer(uint16_t port) {
    if (mHttpServer != nullptr && !StopHttpServer()) { // try stopping the HTTP server if it's already on
        return -2;
    }

    mHttpServer = new HttpServer::HttpServer("httpserver");
    return mHttpServer->Start(port);
}

int Core::StopHttpServer() {
    if (mHttpServer != nullptr && !mHttpServer->Stop()) {
        return 0;
    }
    NOOBWARRIOR_FREE_PTR(mHttpServer)
    return 1;
}
