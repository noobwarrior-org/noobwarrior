// === noobWarrior ===
// File: TestHandler.h
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#pragma once
#include "Handler.h"

namespace NoobWarrior::HttpServer {
class TestHandler : public Handler {
public:
    TestHandler() = default;
    int OnRequest(mg_connection *conn, void *userdata) override;
};
}