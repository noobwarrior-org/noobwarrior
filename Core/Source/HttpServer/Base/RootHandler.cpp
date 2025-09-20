// === noobWarrior ===
// File: RootHandler.cpp
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>

#include <civetweb.h>
#include <nlohmann/json.hpp>
#include <inja/inja.hpp>

#include <fstream>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

RootHandler::RootHandler(HttpServer *server) : mServer(server) {
    
}

int RootHandler::OnRequest(mg_connection *conn, void *userdata) {
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    std::filesystem::path file_path = mServer->GetFilePath(request_info->local_uri);
    if (file_path.empty()) {
        std::string pageName;
        std::string pageOutput;

        // Page should either be a 404 if the user intentionally tried to load in a non-empty URL that doesnt exist
        // But if its just the root point and nothing more, follow standard convention and load the home page.
        pageName = *request_info->local_uri == '/' && *(request_info->local_uri + 1) == '\0' ? "home.jinja" : "not_found.jinja";
        RenderResponse res = mServer->RenderPage(pageName, mServer->GetBaseContextData(), &pageOutput);

        if (res != RenderResponse::Success) {
            mg_send_http_error(conn, 500, "Failed to render page");
            return 500;
        }

        mg_send_http_ok(conn, "text/html", pageOutput.length());
	    mg_write(conn, pageOutput.c_str(), pageOutput.length());
        return 404;
    }
    mg_send_mime_file(conn, file_path.c_str(), nullptr);
    return 200;
}