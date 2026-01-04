// === noobWarrior ===
// File: TestHandler.h
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#pragma once
#include "Handler.h"

namespace NoobWarrior {
class TestHandler : public Handler {
public:
    TestHandler() = default;
    void OnRequest(evhttp_request *req, void *userdata) override;
};
}