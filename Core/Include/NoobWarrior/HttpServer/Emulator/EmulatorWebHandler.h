// === noobWarrior ===
// File: EmulatorWebHandler.h
// Started by: Hattozo
// Started on: 9/7/2025
// Description:
#include <NoobWarrior/HttpServer/Base/WebHandler.h>

namespace NoobWarrior::HttpServer {
class HttpServer;
class EmulatorWebHandler : public WebHandler {
public:
    EmulatorWebHandler(HttpServer *server);
};
}