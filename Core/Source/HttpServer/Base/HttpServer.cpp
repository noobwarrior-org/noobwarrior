// === noobWarrior ===
// File: HttpServer.cpp
// Started by: Hattozo
// Started on: 3/10/2025
// Description: A HTTP server that is responsible for mimicking the Roblox API and serving files from noobWarrior archives
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/TestHandler.h>
#include <NoobWarrior/HttpServer/Base/WebHandler.h>
#include <NoobWarrior/Macros.h>
#include <NoobWarrior/Log.h>

#include <filesystem>
#include <cassert>

#include "NoobWarrior/NoobWarrior.h"

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
    auto arrayOfData = (void**)userdata;
    auto handler = (Handler*)arrayOfData[0]; // handler should be the first thing passed to the void pointer array.

    typedef int (Handler::*classFunc_t)(mg_connection*, void*);
    classFunc_t classFunc = &Handler::OnRequest;

    void *userdataFromArray = arrayOfData[1]; // and also any extra userdata.

    return (handler->*classFunc)(conn, userdataFromArray);
}

int HttpServer::Start(uint16_t port) {
    char portStr[5];
    snprintf(portStr, 5, "%i", port);

    const std::filesystem::path &staticDir = Directory / "web" / "static";
    const std::string &staticDirStr = staticDir.generic_string();

    const char* configOptions[] = {"listening_ports", portStr, nullptr};
    Server = mg_start(nullptr, nullptr, configOptions);

    mRootHandler = new RootHandler(this);
    mWebHandler = new WebHandler(this);
    
    SetRequestHandler("/", mRootHandler);

    Out(Name, "Started server on port {}", port);
    Running = true;
    return 1;
}

int HttpServer::Stop() {
    Running = false;
    Out(Name, "Stopping server...");

    mg_stop(Server);
    Server = nullptr;

    NOOBWARRIOR_FREE_PTR(mWebHandler)
    NOOBWARRIOR_FREE_PTR(mRootHandler)
    for (void** arr : HandlerUserdata) {
        NOOBWARRIOR_FREE_PTR(arr)
    }
    return 1;
}

void HttpServer::SetRequestHandler(const char *uri, Handler *handler, void *userdata) {
    // pass an array containing our handler object and user data so that it knows what the object is.
    // allocate it on heap too so that we still have it even when this function is done, because this request handler listener will be called later.
    void** userdataArray = new void*[2]{handler, userdata};
    HandlerUserdata.push_back(userdataArray); // remember to push this to our vector array so that we know that we have to free it during cleanup
    mg_set_request_handler(Server, uri, CFuncToObjectFuncHandler, userdataArray);
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

bool HttpServer::IsRunning() {
    return Running;
}
