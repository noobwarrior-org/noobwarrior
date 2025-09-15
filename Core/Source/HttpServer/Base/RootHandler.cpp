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

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

RootHandler::RootHandler(HttpServer *server) : mServer(server) {
    
}

int RootHandler::OnRequest(mg_connection *conn, void *userdata) {
    const struct mg_request_info *request_info = mg_get_request_info(conn);
    std::filesystem::path file_path = mServer->GetFilePath(request_info->local_uri);
    if (file_path.empty()) {
        std::filesystem::path notfound_template = mServer->GetFilePath("not_found.jinja", "templates");
        std::filesystem::path main_template = mServer->GetFilePath("main.jinja", "templates");

        std::istream stream(notfound_template);

        nlohmann::json data;
        data["body"] = main_template;

        inja::render(main_template, data);
        mg_send_http_error(conn, 404, "Could not find file %s", request_info->local_uri);
        return 404;
    }
    mg_send_mime_file(conn, file_path.c_str(), nullptr);
    return 200;
}