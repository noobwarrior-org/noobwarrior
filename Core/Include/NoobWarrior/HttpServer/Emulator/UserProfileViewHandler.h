// === noobWarrior ===
// File: UserProfileViewHandler.h
// Started by: Hattozo
// Started on: 10/4/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>

namespace NoobWarrior::HttpServer {
class UserProfileViewHandler : public WebHandler {
public:
    UserProfileViewHandler(ServerEmulator *server);
protected:
    nlohmann::json GetContextData(mg_connection *conn = nullptr) override;
};
}
