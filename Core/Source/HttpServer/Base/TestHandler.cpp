// === noobWarrior ===
// File: TestHandler.cpp
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#include <NoobWarrior/HttpServer/Base/TestHandler.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

int TestHandler::OnRequest(mg_connection *conn, void *userdata) {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/plain\r\nConnection: close\r\n\r\n");
	mg_printf(conn, "Test!");
    return 200;
}