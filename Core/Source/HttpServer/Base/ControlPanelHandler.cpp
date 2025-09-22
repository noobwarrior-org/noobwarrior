// === noobWarrior ===
// File: ControlPanelHandler.cpp
// Started by: Hattozo
// Started on: 9/20/2025
// Description:
#include <NoobWarrior/HttpServer/Base/ControlPanelHandler.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ControlPanelHandler::ControlPanelHandler(HttpServer *server) : mServer(server) {
    
}

int ControlPanelHandler::OnRequest(mg_connection *conn, void *userdata) {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: "
	          "text/plain\r\nConnection: close\r\n\r\n");
	mg_printf(conn, "Test!");
    return 200;
}