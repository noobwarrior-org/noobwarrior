// === noobWarrior ===
// File: WebHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Log.h>

#include <civetweb.h>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

WebHandler::WebHandler(HttpServer *server) : mServer(server) {}

int WebHandler::OnRequest(mg_connection *conn, void *userdata) {
    const mg_request_info *request_info = mg_get_request_info(conn);
    const char* cookie_header = mg_get_header(conn, "Cookie");
    char session_token[1024];
    if (cookie_header) mg_get_cookie(cookie_header, ".LOGINSESSION", session_token, sizeof(session_token));

    char *fileName = static_cast<char*>(userdata);

#ifdef DEBUG
    Out("HttpServer::WebHandler", "Accessing file {}", fileName);
#endif

    Config *config = mServer->mCore->GetConfig();

    std::string pageOutput;
    RenderResponse res = mServer->RenderPage(fileName, GetContextData(), &pageOutput);

    std::string resMsg = "Unknown error";
    int resCode = 500;
    switch (res) {
    default: break;
    case RenderResponse::FailedOpeningTemplateFile:
        resMsg = "Failed to open template file on server";
        break;
    case RenderResponse::FailedRenderingBody:
        resMsg = "Failed to render the body of this page";
        break;
    case RenderResponse::FailedRenderingMain:
        resMsg = "Failed to render the main section of this page";
        break;
    }

    if (res != RenderResponse::Success) {
        mg_send_http_error(conn, resCode, res == RenderResponse::FailedRenderingMain || res == RenderResponse::FailedRenderingBody ? "%s\nJinja error message: \"%s\"" : "%s", resMsg.c_str(), pageOutput.c_str());
        return 500;
    }

    resMsg = "OK";
    resCode = 200;

    mg_send_http_ok(conn, "text/html", pageOutput.length());
	mg_write(conn, pageOutput.c_str(), pageOutput.length());
    return resCode;
}

nlohmann::json WebHandler::GetContextData(mg_connection *conn) {
    return mServer->GetBaseContextData(conn);
}