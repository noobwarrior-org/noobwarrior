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

    std::filesystem::path serverDir = mServer->Directory / mServer->DirName;

    std::filesystem::path mainFilePath = serverDir / "templates" / "main.jinja";
    std::filesystem::path filePath = serverDir / "templates" / fileName;

    std::string mainFileString = mainFilePath.string();
    std::string fileString = filePath.string();

    std::ifstream mainFileInput;
    std::ifstream fileInput;

    mainFileInput.open(mainFilePath);
    fileInput.open(filePath);

    if (mainFileInput.fail() || fileInput.fail()) {
        mg_send_http_error(conn, 500, "Failed to open \"%ls\". The website cannot be accessed.", mainFileInput.fail() ? mainFileString.c_str() : fileString.c_str());
        return 500;
    }

    if (!session_token) {
        
    }

    nlohmann::json data = GetContextData();

    std::stringstream bodyFileBuf;
    bodyFileBuf << fileInput.rdbuf();

    std::string generatedBodyTemplate;
    try {
        generatedBodyTemplate = inja::render(bodyFileBuf.str(), data);
    } catch (const inja::InjaError &e) {
        OutEx(&std::cerr, "HttpServer::WebHandler", "Failed to render body template \"{}\" for IP address {}\nException Message: {}", fileName, request_info->remote_addr, e.what());
        mg_send_http_error(conn, 500, "Failed to render body template \"%s\"\nException Message: %s", fileName, e.what());
        return 500;
    }
    
    // now generate the main template, and send it to client
    std::stringstream mainFileBuf;
    mainFileBuf << mainFileInput.rdbuf();

    data["body"] = generatedBodyTemplate;

    std::string generatedMainTemplate;
    try {
        generatedMainTemplate = inja::render(mainFileBuf.str(), data);
    } catch (const inja::InjaError &e) {
        OutEx(&std::cerr, "HttpServer::WebHandler", "Failed to render body template \"{}\" for IP address {}\nException Message: {}", fileName, request_info->remote_addr, e.what());
        mg_send_http_error(conn, 500, "Failed to render main template\nException Message: %s", e.what());
        return 500;
    }

    mg_send_http_ok(conn, "text/html", generatedMainTemplate.length());
	mg_write(conn, generatedMainTemplate.c_str(), generatedMainTemplate.length());

    // cleanup
    mainFileInput.close();
    fileInput.close();
    return 200;
}

nlohmann::json WebHandler::GetContextData() {
    Config *config = mServer->mCore->GetConfig();

    std::optional title = config->GetKeyValue<std::string>("httpserver.branding.title");
    std::optional logo = config->GetKeyValue<std::string>("httpserver.branding.logo");
    std::optional favicon = config->GetKeyValue<std::string>("httpserver.branding.favicon");

    nlohmann::json data;
    data["title"] = "RegularPage";
    data["branding"]["title"] = title.has_value() ? title.value() : "noobWarrior";
    data["branding"]["logo"] = logo.has_value() ? logo.value() : "/img/icon1024.png";
    data["branding"]["favicon"] = favicon.has_value() ? favicon.value() : "/img/favicon.ico";

    data["user"]["id"] = -1;
    data["user"]["name"] = "Guest";
    data["user"]["rank"] = "guest";
    data["user"]["headshot_thumbnail"] = "";

    return data;
}