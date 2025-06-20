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

WebHandler::WebHandler(const std::filesystem::path &dir) : Directory(dir) {}

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
        mg_send_http_error(conn, 500, "Failed to open \"%ls\". The website cannot be accessed.", mainFileInput.fail() ? mainFilePath.c_str() : filePath.c_str());
        return 500;
    }

    // generate the body template
    nlohmann::json bodyData;

    std::stringstream bodyFileBuf;
    bodyFileBuf << fileInput.rdbuf();

    std::string generatedBodyTemplate = inja::render(bodyFileBuf.str(), bodyData);
    
    // now generate the main template, and send it to client
    std::stringstream mainFileBuf;
    mainFileBuf << mainFileInput.rdbuf();

    nlohmann::json mainData;
    mainData["body"] = generatedBodyTemplate;
    mainData["title"] = "RegularPage";

    std::string generatedMainTemplate = inja::render(mainFileBuf.str(), mainData);

    mg_send_http_ok(conn, "text/html", generatedMainTemplate.length());
	mg_write(conn, generatedMainTemplate.c_str(), generatedMainTemplate.length());

    Out("HttpServer::WebHandler", "Wrote to client {}", generatedMainTemplate);

    // cleanup
    mainFileInput.close();
    fileInput.close();
    return 200;
}