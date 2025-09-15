// === noobWarrior ===
// File: EmulatorWebHandler.h
// Started by: Hattozo
// Started on: 9/7/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/WebHandler.h>

namespace NoobWarrior::HttpServer {
class ServerEmulator;
class EmulatorWebHandler : public WebHandler {
public:
    EmulatorWebHandler(ServerEmulator *server);
    nlohmann::json GetContextData() override;
protected:
    ServerEmulator *mServer;
};
}