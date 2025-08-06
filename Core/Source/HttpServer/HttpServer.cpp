// === noobWarrior ===
// File: HttpServer.cpp
// Started by: Hattozo
// Started on: 3/10/2025
// Description: A HTTP server that is responsible for mimicking the Roblox API and serving files from noobWarrior archives
#include <NoobWarrior/HttpServer/HttpServer.h>
#include <NoobWarrior/Macros.h>
#include <NoobWarrior/Log.h>

#include <filesystem>
#include <cassert>

#include "NoobWarrior/NoobWarrior.h"

#define SET_URI(uri, handler)
#define LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler, (void*)fileName);

using namespace NoobWarrior::HttpServer;

HttpServer::HttpServer(Core *core) :
    mCore(core),
    Directory(mCore->GetInstallationDir() / "httpserver"),
    Server(nullptr)
{}

static int CFuncToObjectFuncHandler(struct mg_connection *conn, void *userdata) {
    auto arrayOfData = (void**)userdata;
    auto handler = (Handler*)arrayOfData[0]; // handler should be the first thing passed to the void pointer array.

    typedef int (Handler::*classFunc_t)(mg_connection*, void*);
    classFunc_t classFunc = &Handler::OnRequest;

    void *userdataFromArray = arrayOfData[1]; // and also any extra userdata.

    return (handler->*classFunc)(conn, userdataFromArray);
}

void HttpServer::SetRequestHandler(const char *uri, Handler *handler, void *userdata) {
    // pass an array containing our handler object and user data so that it knows what the object is.
    // allocate it on heap too so that we still have it even when this function is done, because this request handler listener will be called later.
    void** userdataArray = new void*[2]{handler, userdata};
    HandlerUserdata.push_back(userdataArray); // remember to push this to our vector array so that we know that we have to free it during cleanup
    mg_set_request_handler(Server, uri, CFuncToObjectFuncHandler, userdataArray);
}

int HttpServer::Start(uint16_t port) {
    // GetConfig()->Open();
    // GetConfig()->Port = port;

    // yes i am aware civetweb has a c++ interface, i would rather just interact with the regular C interface instead.
    char portStr[5];
    snprintf(portStr, 5, "%i", port);

    const std::filesystem::path &staticDir = Directory / "web" / "static";
    const std::string &staticDirStr = staticDir.generic_string();

    const char* configOptions[] = {"listening_ports", portStr, "document_root", staticDirStr.c_str(), nullptr};
    Server = mg_start(nullptr, nullptr, configOptions);

    mWebHandler = new WebHandler(Directory);
    mAssetHandler = new AssetHandler(this, mCore->GetDatabaseManager());

    SetRequestHandler("/asset", mAssetHandler);
    SetRequestHandler("/v1/asset", mAssetHandler);

    // LINK_URI_TO_TEMPLATE("/", "guest.jinja")
    LINK_URI_TO_TEMPLATE("/login", "login.jinja")
    LINK_URI_TO_TEMPLATE("/home", "home.jinja")

    Out("HttpServer", "Started server on port {}", port);
    return 1;
}

int HttpServer::Stop() {
    Out("HttpServer", "Stopping server...");

    // GetConfig()->Close();
    // NOOBWARRIOR_FREE_PTR(mConfig)

    mg_stop(Server);
    Server = nullptr;

    NOOBWARRIOR_FREE_PTR(mAssetHandler)
    NOOBWARRIOR_FREE_PTR(mWebHandler)
    for (void** arr : HandlerUserdata) {
        NOOBWARRIOR_FREE_PTR(arr)
    }
    return 1;
}
