// === noobWarrior ===
// File: ContentPageHandler.h
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/WebHandler.h>

namespace NoobWarrior::HttpServer {
class ServerEmulator;
class ContentPageHandler : public WebHandler {
public:
    ContentPageHandler(ServerEmulator *server);
protected:
    nlohmann::json GetContextData() override;
};
}