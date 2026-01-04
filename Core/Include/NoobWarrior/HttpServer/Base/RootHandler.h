// === noobWarrior ===
// File: RootHandler.h
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#pragma once
#include "Handler.h"

namespace NoobWarrior {
class HttpServer;
class RootHandler : public Handler {
public:
    RootHandler(HttpServer *server);
    void OnRequest(evhttp_request *req, void *userdata) override;
protected:
    HttpServer *mServer;
};
}