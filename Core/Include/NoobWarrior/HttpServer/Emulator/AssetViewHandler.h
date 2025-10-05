// === noobWarrior ===
// File: AssetViewHandler.h
// Started by: Hattozo
// Started on: 10/4/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>

namespace NoobWarrior::HttpServer {
class AssetViewHandler : public WebHandler {
public:
    AssetViewHandler(ServerEmulator *server);
protected:
    nlohmann::json GetContextData(mg_connection *conn = nullptr) override;
};
}