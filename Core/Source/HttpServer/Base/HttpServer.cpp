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

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/bufferevent_ssl.h>

using namespace NoobWarrior;

static bufferevent* bevcb(struct event_base *base, void *arg) {
    bufferevent* r;
    if (arg == nullptr) {
        r = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    } else {
        SSL_CTX* ssl_ctx = static_cast<SSL_CTX*>(arg);
        // glad this is built-in
        r = bufferevent_openssl_socket_new(base, -1, SSL_new(ssl_ctx), BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
    }
    return r;
}

// Example taken from https://linux.die.net/man/3/ssl_ctx_set_default_passwd_cb
static int pem_passwd_cb(char *buf, int size, int rwflag, void *password)
{
    strncpy(buf, (char *)(password), size);
    buf[size - 1] = '\0';
    return(strlen(buf));
}

HttpServer::HttpServer(Core *core, std::string logName) :
    mRunning(false),
    mLogName(std::move(logName)),
    mCore(core),
    mServer(nullptr),
    mVfs(new OverlayFileSystem()),

    mSslCtx(nullptr),
    mServerSecure(nullptr),
    mRunningSecure(false),
    
    mRootHandler(this),
    mTestHandler()
{}

HttpServer::~HttpServer() {
    HttpServer::Stop();
    HttpServer::StopSecure();
    NOOBWARRIOR_FREE_PTR(mVfs)
}

static void CFuncToObjectFuncHandler(struct evhttp_request *req, void *userdata) {
    auto tuple = static_cast<std::tuple<Handler*, void*>*>(userdata);
    Handler* handler = std::get<0>(*tuple);
    void* data = std::get<1>(*tuple);
    handler->OnRequest(req, data);
}

void HttpServer::SetupHandlers() {
    SetRequestHandler(nullptr, &mRootHandler);
    SetRequestHandler("/test", &mTestHandler);
}

void HttpServer::ApplyHandlersToServer(evhttp *server) {
    for (const auto &entry : mStoredHandlers) {
        if (!entry.uri)
            evhttp_set_gencb(server, CFuncToObjectFuncHandler, entry.raw);
    }
}

int HttpServer::Start(uint16_t port) {
    if (mRunning)
        return 0;

    mPreStartSignal.Fire(false); // We're passing "false" because this is the insecure variant of the server
    mServer = evhttp_new(mCore->GetEventBase());
    evhttp_set_allowed_methods(mServer, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_HEAD |
        EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE | EVHTTP_REQ_OPTIONS | EVHTTP_REQ_PATCH);
    evhttp_bind_socket(mServer, "0.0.0.0", port);

    if (!mHandlersSetUp) {
        SetupHandlers();
        mHandlersSetUp = true;
    } else {
        ApplyHandlersToServer(mServer);
    }

    Out(mLogName, "Started HTTP server on port {}", port);
    mRunning = true;
    mPostStartSignal.Fire(false); // We're passing "false" because this is the insecure variant of the server
    return 1;
}

int HttpServer::Stop() {
    if (!mRunning)
        return 0;

    mPreStopSignal.Fire(false); // We're passing "false" because this is the insecure variant of the server
    mRunning = false;
    Out(mLogName, "Stopping HTTP server...");

    evhttp_free(mServer);
    mServer = nullptr;
    if (!mRunningSecure) {
        HandlerUserdata.clear();
        mStoredHandlers.clear();
        mHandlersSetUp = false;
    }
    mPostStopSignal.Fire(false); // We're passing "false" because this is the insecure variant of the server
    return 1;
}

int HttpServer::StartSecure(uint16_t port) {
    if (mRunningSecure)
        return 0;

    mPreStartSignal.Fire(true); // We're passing "true" because this is the secure variant of the server

    if (mSslCtx == nullptr) {
        mSslCtx = SSL_CTX_new(TLS_server_method());
        if (!mSslCtx) {
            Out(mLogName, "Failed to initialize OpenSSL context");
            return -1;
        }

        // Out(mLogName, "OpenSSL: Using passphrase \"noobwarrior\"");
        // SSL_CTX_set_default_passwd_cb(mSslCtx, pem_passwd_cb);
        // SSL_CTX_set_default_passwd_cb_userdata(mSslCtx, (void*)"farted");

        std::filesystem::path certPath = mCore->GetUserDataDir() / NW_PATH_SSL / "cert.pem";
        std::filesystem::path keyPath = mCore->GetUserDataDir() / NW_PATH_SSL / "key.pem";


#if defined(_WIN32)
        if (!SSL_CTX_use_certificate_file(mSslCtx, WideCharToUTF8((wchar_t*)certPath.c_str()).c_str(), SSL_FILETYPE_PEM)) {
#else
        if (!SSL_CTX_use_certificate_file(mSslCtx, certPath.c_str(), SSL_FILETYPE_PEM)) {
#endif
            Out(mLogName, "OpenSSL: Failed to use public key certificate \"cert.pem\"!");
            SSL_CTX_free(mSslCtx);
            mSslCtx = nullptr;
            return -2;
        }
#if defined(_WIN32)
        if (!SSL_CTX_use_PrivateKey_file(mSslCtx, WideCharToUTF8((wchar_t*)keyPath.c_str()).c_str(), SSL_FILETYPE_PEM)) {
#else
        if (!SSL_CTX_use_PrivateKey_file(mSslCtx, keyPath.c_str(), SSL_FILETYPE_PEM)) {
#endif
            Out(mLogName, "OpenSSL: Failed to use private key \"key.pem\"! Maybe the passphrase is incorrect?");
            SSL_CTX_free(mSslCtx);
            mSslCtx = nullptr;
            return -3;
        }
    }

    mServerSecure = evhttp_new(mCore->GetEventBase());
    evhttp_set_allowed_methods(mServerSecure, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_HEAD |
        EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE | EVHTTP_REQ_OPTIONS | EVHTTP_REQ_PATCH);
    evhttp_bind_socket(mServerSecure, "0.0.0.0", port);
    evhttp_set_bevcb(mServerSecure, bevcb, mSslCtx);

    if (!mHandlersSetUp) {
        SetupHandlers();
        mHandlersSetUp = true;
    } else {
        ApplyHandlersToServer(mServerSecure);
    }

    Out(mLogName, "Started HTTPS server on port {}", port);
    mRunningSecure = true;
    mPostStartSignal.Fire(true); // We're passing "true" because this is the secure variant of the server
    return 1;
}

int HttpServer::StopSecure() {
    if (!mRunningSecure)
        return 0;

    if (mSslCtx != nullptr) {
        SSL_CTX_free(mSslCtx);
        mSslCtx = nullptr;
    }

    /*if (mEcKeyPair != nullptr) {
        EVP_PKEY_free(mEcKeyPair);
        mEcKeyPair = nullptr;
    }*/

    mPreStopSignal.Fire(true); // We're passing "true" because this is the secure variant of the server
    mRunningSecure = false;
    Out(mLogName, "Stopping HTTPS server...");

    evhttp_free(mServerSecure);
    mServerSecure = nullptr;
    if (!mRunning) {
        HandlerUserdata.clear();
        mStoredHandlers.clear();
        mHandlersSetUp = false;
    }
    mPostStopSignal.Fire(true); // We're passing "true" because this is the secure variant of the server
    return 1;
}

void HttpServer::SetRequestHandler(const char *uri, Handler *handler, void *userdata) {
    // allocate on heap so the pointer remains valid when libevent fires the callback later
    auto handler_userdata_pair = std::make_unique<std::tuple<Handler*, void*>>(handler, userdata);
    auto *raw = handler_userdata_pair.get();
    HandlerUserdata.push_back(std::move(handler_userdata_pair));
    mStoredHandlers.push_back({uri ? std::optional<std::string>(uri) : std::nullopt, raw});
    if (uri == nullptr) {
        if (mServer != nullptr)
            evhttp_set_gencb(mServer, CFuncToObjectFuncHandler, static_cast<void*>(raw));
        if (mServerSecure != nullptr)
            evhttp_set_gencb(mServerSecure, CFuncToObjectFuncHandler, static_cast<void*>(raw));
    }
}

const std::map<std::string, std::string>& HttpServer::GetRouteParams() const {
    return mRouteParams;
}

std::string HttpServer::GetRouteParam(const std::string &name) const {
    auto it = mRouteParams.find(name);
    return it == mRouteParams.end() ? std::string() : it->second;
}

VirtualFileSystem::Response HttpServer::MountVolume(const std::string &root, const Url &urlPath) {
    Out(mLogName, "Mounting {} to volume {}", urlPath.Resolve(), root);
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
    return mRunning || mRunningSecure;
}

Core *HttpServer::GetCore() {
    return mCore;
}

OverlayFileSystem* HttpServer::GetVfs() {
    return mVfs;
}
