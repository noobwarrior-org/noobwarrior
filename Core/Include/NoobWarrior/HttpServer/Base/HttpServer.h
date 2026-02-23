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

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <memory>
#include <utility>
#include <tuple>

#include <evhttp.h>
#include <nlohmann/json_fwd.hpp>

#define NOOBWARRIOR_SET_URI(uri, handler)
#define NOOBWARRIOR_LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler.get(), (void*)fileName);

namespace NoobWarrior { class Core; }
namespace NoobWarrior {
enum class RenderResponse {
    Failed,
    Success,
    FailedRenderingBody,
    FailedRenderingMain,
    FailedOpeningTemplateFile
};

class HttpServer {
// TODO: The comments in this class are really shit and explains the properties really poorly.
// Someone needs to word it better than I can
    friend class RootHandler;
    friend class WebHandler;
public:
    HttpServer(Core *core, std::string logName = "HttpServer");
    virtual ~HttpServer();
    
    virtual int Start(uint16_t port);
    virtual int Stop();

    bool        IsRunning();
    void        SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);

    LuaSignal* GetOnRequestSignal();

    Core *GetCore();
protected:
    bool Running;

    // This is used in logging.
    std::string LogName;
    
    Core *mCore;
    
    evhttp* Server;

    //////////////// Handlers ////////////////
    std::unique_ptr<RootHandler> mRootHandler;
    std::unique_ptr<TestHandler> mTestHandler;

    std::vector<std::unique_ptr<std::tuple<Handler*, void*>>> HandlerUserdata;

    //////////////// Signals ////////////////
    LuaSignal mOnRequestSignal;
};
}