/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: HttpServer.cpp
// Started by: Hattozo
// Started on: 3/10/2025
// Description: A HTTP server.
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/HttpServer/Base/RootHandler.h>
#include <NoobWarrior/HttpServer/Base/TestHandler.h>
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Macros.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior;

HttpServer::HttpServer(Core *core, std::string logName) :
    Running(false),
    LogName(std::move(logName)),
    mCore(core),
    Server(nullptr),
    mVfs(new OverlayFileSystem())
{}

HttpServer::~HttpServer() {
    Stop();
    NOOBWARRIOR_FREE_PTR(mVfs)
}

static void CFuncToObjectFuncHandler(struct evhttp_request *req, void *userdata) {
    auto tuple = static_cast<std::tuple<Handler*, void*>*>(userdata);
    Handler* handler = std::get<0>(*tuple);
    void* data = std::get<1>(*tuple);
    handler->OnRequest(req, data);
}

int HttpServer::Start(uint16_t port) {
    if (Running)
        return 0;

    mPreStartSignal.Fire();
    Server = evhttp_new(mCore->GetEventBase());
    evhttp_bind_socket(Server, "0.0.0.0", port);

    mRootHandler = std::make_unique<RootHandler>(this);
    mTestHandler = std::make_unique<TestHandler>();
    
    SetRequestHandler(nullptr, mRootHandler.get());
    SetRequestHandler("/test", mTestHandler.get());

    Out(LogName, "Started server on port {}", port);
    Running = true;
    mPostStartSignal.Fire();
    return 1;
}

int HttpServer::Stop() {
    if (!Running)
        return 0;

    mPreStopSignal.Fire();
    Running = false;
    Out(LogName, "Stopping server...");

    evhttp_free(Server);
    Server = nullptr;
    HandlerUserdata.clear();
    mPostStopSignal.Fire();
    return 1;
}

void HttpServer::SetRequestHandler(const char *uri, Handler *handler, void *userdata) {
    // pass a std pair containing our handler object and user data so that it knows what the object is.
    // allocate it on heap too so that we still have it even when this function is done, because this request handler listener will be called later.
    auto handler_userdata_pair = std::make_unique<std::tuple<Handler*, void*>>(handler, userdata);
    auto *raw = handler_userdata_pair.get();
    HandlerUserdata.push_back(std::move(handler_userdata_pair));
    if (uri != nullptr)
        evhttp_set_cb(Server, uri, CFuncToObjectFuncHandler, static_cast<void*>(raw));
    else
        evhttp_set_gencb(Server, CFuncToObjectFuncHandler, static_cast<void*>(raw));
}

VirtualFileSystem::Response HttpServer::MountVolume(const std::string &root, const Url &urlPath) {
    Out("HttpServer", "Mounting {} to volume {}", urlPath.Resolve(), root);
    std::unique_ptr<VirtualFileSystem> vfs;
    std::filesystem::path path = urlPath.ResolveAsLocalPath(mCore);
    if (path.extension().compare(".zip") == 0) {
        vfs = std::make_unique<ZipFileSystem>(path);
    } else vfs = std::make_unique<StdFileSystem>(path);
    return mVfs->Mount(root, std::move(vfs));
}

VirtualFileSystem::Response HttpServer::UnmountVolume(const std::string &root, const Url &urlPath) {
    return VirtualFileSystem::Response::Failed;
}

LuaSignal* HttpServer::GetPreStartSignal() {
    return &mPreStartSignal;
}

LuaSignal* HttpServer::GetPreStopSignal() {
    return &mPreStopSignal;
}

LuaSignal* HttpServer::GetPostStartSignal() {
    return &mPostStartSignal;
}

LuaSignal* HttpServer::GetPostStopSignal() {
    return &mPostStopSignal;
}

LuaSignal* HttpServer::GetOnRequestSignal() {
    return &mOnRequestSignal;
}

bool HttpServer::IsRunning() {
    return Running;
}

NoobWarrior::Core *HttpServer::GetCore() {
    return mCore;
}

OverlayFileSystem* HttpServer::GetVfs() {
    return mVfs;
}
