// === noobWarrior ===
// File: WebHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#include <NoobWarrior/HttpServer/WebHandler.h>
#include <NoobWarrior/Log.h>

#include <civetweb.h>
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

WebHandler::WebHandler(Config *config, const std::filesystem::path &dir) : mConfig(config), Directory(dir) {}

int WebHandler::OnRequest(mg_connection *conn, void *userdata) {
    char *fileName = static_cast<char*>(userdata);

#ifdef DEBUG
    Out("HttpServer::WebHandler", "Accessing file {}", fileName);
#endif

    std::filesystem::path mainFilePath = Directory / "web/templates/main.jinja";
    std::filesystem::path filePath = Directory / "web/templates/" / fileName;

    std::ifstream mainFileInput;
    std::ifstream fileInput;

    mainFileInput.open(mainFilePath);
    fileInput.open(filePath);

    if (mainFileInput.fail() || fileInput.fail()) {
        mg_send_http_error(conn, 500, "Failed to open \"%s\". The website cannot be accessed.", mainFileInput.fail() ? mainFilePath.c_str() : filePath.c_str());
        return 500;
    }

    std::optional title = mConfig->GetKeyValue<std::string>("httpserver.branding.title");
    std::optional logo = mConfig->GetKeyValue<std::string>("httpserver.branding.logo");
    std::optional favicon = mConfig->GetKeyValue<std::string>("httpserver.branding.favicon");

    std::optional game_view_mode = mConfig->GetKeyValue<std::string>("httpserver.game_view_mode");

    // generate the template
    nlohmann::json data;
    data["title"] = "RegularPage";
    data["branding"]["title"] = title.has_value() ? title.value() : "noobWarrior";
    data["branding"]["logo"] = logo.has_value() ? logo.value() : "/img/icon1024.png";
    data["branding"]["favicon"] = favicon.has_value() ? favicon.value() : "/img/favicon.ico";
    data["game_view_mode"] = game_view_mode.has_value() ? game_view_mode.value() : "server";

    data["servers"] = {
        {
            {"name", "My Server"}
        }
    };

    std::stringstream bodyFileBuf;
    bodyFileBuf << fileInput.rdbuf();

    std::string generatedBodyTemplate;
    try {
        generatedBodyTemplate = inja::render(bodyFileBuf.str(), data);
    } catch (const inja::InjaError &e) {
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
        mg_send_http_error(conn, 500, "Failed to render main template\nException Message: %s", e.what());
        return 500;
    }

    mg_send_http_ok(conn, "text/html", generatedMainTemplate.length());
	mg_write(conn, generatedMainTemplate.c_str(), generatedMainTemplate.length());

    Out("HttpServer::WebHandler", "Wrote to client {}", generatedMainTemplate);

    // cleanup
    mainFileInput.close();
    fileInput.close();
    return 200;
}