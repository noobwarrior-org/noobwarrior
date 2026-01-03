// === noobWarrior ===
// File: TestHandler.cpp
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#include <NoobWarrior/HttpServer/Base/TestHandler.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

void TestHandler::OnRequest(evhttp_request *req, void *userdata) {
    struct evbuffer *reply = evbuffer_new();
    evbuffer_add_printf(reply, "Test!");
    evhttp_send_reply(req, HTTP_OK, NULL, reply);
    evbuffer_free(reply);
}