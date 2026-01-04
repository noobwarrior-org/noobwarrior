// === noobWarrior ===
// File: Handler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once

#include <evhttp.h>

namespace NoobWarrior {
class Handler {
public:
    virtual ~Handler() = default;

    virtual void OnRequest(evhttp_request *req, void *userdata) = 0;
};
}