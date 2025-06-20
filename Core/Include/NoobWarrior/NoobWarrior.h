// === noobWarrior ===
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Core of the library
#pragma once

#include "Macros.h"
#include "Log.h"
#include "Database.h"
#include "Auth.h"
#include "Config.h"
#include "RccServiceManager.h"
#include "Roblox/DataModel/RobloxFile.h"
#include "HttpServer/HttpServer.h"

#include <vector>

namespace NoobWarrior {
typedef struct {
    const char *ConfigFile;
    const char *AuthFile;
} Init;

class Core {
public:
    Core(Init = {});

    int StartHttpServer(uint16_t port = 8080);
    int StopHttpServer();
private:
    HttpServer::HttpServer *mHttpServer;
    std::vector<RccServiceManager*> RccServiceManagers;
};
}
