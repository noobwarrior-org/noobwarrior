// === noobWarrior ===
// File: Handler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once

#include <civetweb.h>

namespace NoobWarrior::HttpServer {
class Handler {
public:
    virtual ~Handler() = default;

    virtual int OnRequest(mg_connection *conn, void *userdata) = 0;
};
}