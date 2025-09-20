// === noobWarrior ===
// File: HttpServer.cpp
// Started by: Hattozo
// Started on: 3/10/2025
// Description: A HTTP server that is responsible for mimicking the Roblox API and serving files from noobWarrior archives
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/TestHandler.h>
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Macros.h>
#include <NoobWarrior/Log.h>

#include <filesystem>
#include <cassert>
#include <sstream>

#include <nlohmann/json.hpp>
#include <inja/inja.hpp>

using namespace NoobWarrior::HttpServer;

HttpServer::HttpServer(Core *core, std::string name, std::string dirName) :
    Running(false),
    Name(std::move(name)),
    DirName(std::move(dirName)),
    mCore(core),
    Server(nullptr)
{
    Directory = mCore->GetInstallationDir() / "httpserver";
}

static int CFuncToObjectFuncHandler(struct mg_connection *conn, void *userdata) {
    auto pair = static_cast<std::pair<Handler*, void*>*>(userdata);
    return pair->first->OnRequest(conn, pair->second);
}

int HttpServer::Start(uint16_t port) {
    char portStr[5];
    snprintf(portStr, 5, "%i", port);

    const std::filesystem::path &staticDir = Directory / "web" / "static";
    const std::string &staticDirStr = staticDir.generic_string();

    const char* configOptions[] = {"listening_ports", portStr, nullptr};
    Server = mg_start(nullptr, nullptr, configOptions);

    mRootHandler = std::make_unique<RootHandler>(this);
    mWebHandler = std::make_unique<WebHandler>(this);
    
    SetRequestHandler("/", mRootHandler.get());

    Out(Name, "Started server on port {}", port);
    Running = true;
    return 1;
}

int HttpServer::Stop() {
    Running = false;
    Out(Name, "Stopping server...");

    mg_stop(Server);
    Server = nullptr;
    return 1;
}

void HttpServer::SetRequestHandler(const char *uri, Handler *handler, void *userdata) {
    // pass a std pair containing our handler object and user data so that it knows what the object is.
    // allocate it on heap too so that we still have it even when this function is done, because this request handler listener will be called later.
    auto handler_userdata_pair = std::make_unique<std::pair<Handler*, void*>>(handler, userdata);
    auto *raw = handler_userdata_pair.get();
    HandlerUserdata.push_back(std::move(handler_userdata_pair));
    mg_set_request_handler(Server, uri, CFuncToObjectFuncHandler, static_cast<void*>(raw));
}

static bool IsPathEscaping(const std::filesystem::path &path) {
    return std::filesystem::weakly_canonical(path).string().compare(path.string()) != 0;
}

std::filesystem::path HttpServer::GetFilePath(std::string relativeFilePath, const std::string &secondDirName) {
    // trim out all slashes at the start of the relative file path to prevent path traversal vulnerabilties
    // this is also required because having a slash at the beginning of a path will fuck everything up when you append it to another path using the slash operator
    // what does it do? it goes all the way back to the root directory
    while (relativeFilePath.starts_with('/'))
        relativeFilePath = relativeFilePath.substr(1);

    std::filesystem::path file_path = (Directory / DirName / secondDirName / relativeFilePath);
    std::filesystem::path file_path_common = (Directory / "common" / secondDirName / relativeFilePath);

    if (IsPathEscaping(file_path) || IsPathEscaping(file_path_common))
        return std::filesystem::path(); // return nothing for safety reasons

    if (!std::filesystem::exists(file_path) && std::filesystem::exists(file_path_common))
        return file_path_common;
    else if (std::filesystem::exists(file_path))
        return file_path;
    else
        return std::filesystem::path();
}

RenderResponse HttpServer::RenderPage(const std::string &pageLoc, nlohmann::json data, std::string *output) {
    std::filesystem::path serverDir = Directory / DirName;

    std::filesystem::path mainFilePath = GetFilePath("main.jinja", "templates");
    std::filesystem::path filePath = GetFilePath(pageLoc, "templates");

    std::string mainFileString = mainFilePath.string();
    std::string fileString = filePath.string();

    std::ifstream mainFileInput;
    std::ifstream fileInput;

    mainFileInput.open(mainFilePath);
    fileInput.open(filePath);

    if (mainFileInput.fail() || fileInput.fail()) {
        return RenderResponse::FailedOpeningTemplateFile;
    }

    std::stringstream bodyFileBuf;
    bodyFileBuf << fileInput.rdbuf();

    std::string generatedBodyTemplate;
    try {
        generatedBodyTemplate = inja::render(bodyFileBuf.str(), data);
    } catch (const inja::InjaError &e) {
        *output = e.message;
        return RenderResponse::FailedRenderingBody;
    }
    
    // now generate the main template, and send it to client
    std::stringstream mainFileBuf;
    mainFileBuf << mainFileInput.rdbuf();

    data["body"] = generatedBodyTemplate;

    std::string generatedMainTemplate;
    try {
        generatedMainTemplate = inja::render(mainFileBuf.str(), data);
    } catch (const inja::InjaError &e) {
        *output = e.message;
        return RenderResponse::FailedRenderingMain;
    }

    *output = generatedMainTemplate;

    // cleanup
    mainFileInput.close();
    fileInput.close();
    return RenderResponse::Success;
}

nlohmann::json HttpServer::GetBaseContextData() {
    Config *config = mCore->GetConfig();

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

bool HttpServer::IsRunning() {
    return Running;
}

NoobWarrior::Core *HttpServer::GetCore() {
    return mCore;
}