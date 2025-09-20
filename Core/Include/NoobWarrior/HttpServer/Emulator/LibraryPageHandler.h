// === noobWarrior ===
// File: LibraryPageHandler.h
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/WebHandler.h>

namespace NoobWarrior::HttpServer {
class ServerEmulator;
class LibraryPageHandler : public WebHandler {
public:
    LibraryPageHandler(ServerEmulator *server);
protected:
    nlohmann::json GetContextData() override;
};
}