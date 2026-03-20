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
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once
#include "Handler.h"
#include "RootHandler.h"
#include "TestHandler.h"

#include <NoobWarrior/Lua/LuaSignal.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>

#include <cstdint>
#include <vector>
#include <memory>
#include <tuple>

#include <evhttp.h>
#include <nlohmann/json_fwd.hpp>

#define NOOBWARRIOR_SET_URI(uri, handler)
#define NOOBWARRIOR_LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler.get(), (void*)fileName);

namespace NoobWarrior { class Core; }
namespace NoobWarrior {
class HttpServer {
    friend class RootHandler;
    friend class WebHandler;
public:
    enum class Response {
        Failed,
        Success
    };

    HttpServer(Core *core, std::string logName = "HttpServer");
    virtual ~HttpServer();
    
    virtual int Start(uint16_t port);
    virtual int Stop();

    bool        IsRunning();
    void        SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);

    VirtualFileSystem::Response MountVolume(const std::string &root, const Url &urlPath);
    VirtualFileSystem::Response UnmountVolume(const std::string &root, const Url &urlPath);

    LuaSignal* GetOnRequestSignal();

    Core *GetCore();
    OverlayFileSystem* GetVfs();
protected:
    bool Running;

    // This is used in logging.
    std::string LogName;
    
    Core *mCore;
    evhttp* Server;
    OverlayFileSystem* mVfs;

    //////////////// Handlers ////////////////
    std::unique_ptr<RootHandler> mRootHandler;
    std::unique_ptr<TestHandler> mTestHandler;

    std::vector<std::unique_ptr<std::tuple<Handler*, void*>>> HandlerUserdata;

    //////////////// Signals ////////////////
    LuaSignal mOnRequestSignal;
};
}