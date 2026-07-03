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
// File: EmulatorProxy.h
// Started by: Hattozo
// Started on: 6/20/2026
// Description: A LAYERED reverse proxy to remote server emulators. The local client is redirected
//              (by noobHook) to the LOCAL emulator for every web request, but when joining a remote
//              host those requests (the join script, assets, avatar appearance) should be answered
//              by the HOST's emulator instead. Layers are stacked: a request is tried against the
//              top layer first, and if that layer doesn't have it (404 / unreachable) it falls
//              through to the layer below, and so on, finally falling back to the caller's local
//              handling. New layers overlay on top of existing ones.
//              Some assistance by Claude Opus 4.8
#pragma once
#include <evhttp.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace NoobWarrior {
class ServerEmulator;

// EmulatorProxy reverse-proxies a request through a stack of remote server emulators.
//
// The stack is ordered top (index 0, tried first) to bottom. A request is forwarded to each layer
// in turn; the first layer that *has* the resource (an HTTP 2xx) wins. A layer that 404s or is
// unreachable is skipped and the next layer down is tried. If every layer misses, the caller's
// local handling (passed to TryProxy as a fallback) runs as the implicit bottom-most layer.
//
// Threading rule (same as AssetHandler): the worker threads ONLY do the blocking network forward.
// Everything that touches the HTTP request or its connection happens on the event-loop thread.
class EmulatorProxy {
public:
    using LocalFallback = std::function<void(evhttp_request *)>;
    // Applied (on the event-loop thread) to the winning layer's 2xx response body before it is sent
    // to the client. Lets a handler merge client-only data into a proxied response — e.g. overlay the
    // local player's identity onto a join script fetched from the host. Returns the body to send.
    using ResponseTransform = std::function<std::vector<unsigned char>(std::vector<unsigned char>)>;

    // A remote emulator in the stack. SessionToken is the joiner's .LOGINSESSION on that host (empty
    // when the host requires no auth); it is forwarded as a Cookie so the host can identify the joiner.
    struct Layer {
        std::string Host;
        uint16_t Port {0};
        std::string SessionToken;
    };

    explicit EmulatorProxy(ServerEmulator *emu);
    ~EmulatorProxy();

    // Layer management (the stack). Safe to call from any thread.
    void PushLayer(const std::string &host, uint16_t port, const std::string &sessionToken = ""); // overlay a new layer on top
    bool PopLayer();                                         // remove the top layer; false if empty
    void RemoveLayer(const std::string &host, uint16_t port);
    void ClearLayers();
    bool HasLayers() const;
    std::vector<Layer> GetLayers() const; // top-first

    // Event-loop thread. If the stack has at least one layer (and proxying is active), takes
    // ownership of the request, forwards it down the stack on a worker thread, and returns true
    // (the reply happens later). If the stack is empty, returns false and the caller answers the
    // request locally. If every layer misses, localFallback (if given) runs as the bottom layer.
    bool TryProxy(evhttp_request *req, LocalFallback localFallback = {}, ResponseTransform transform = {});

    // Lifecycle, mirrors AssetHandler. Pause MUST run before the server's evhttp is freed so an
    // in-flight forward gives up quietly instead of replying to a destroyed connection.
    void Pause();
    void Resume();
private:
    // One request being forwarded down the layer stack. Shared between the event-loop thread and
    // one worker thread; the client's request stays open until OnComplete replies.
    struct ProxyRequest {
        evhttp_request*    Request    {nullptr};
        evhttp_connection* Connection {nullptr};
        std::atomic<bool>  ClientConnected {true}; // set false if the client hangs up mid-forward

        std::string              Method;       // "GET" / "POST" / "PUT"
        std::vector<std::string> Urls;         // one full https URL per layer, top-first
        std::vector<std::string> Cookies;      // per-layer .LOGINSESSION token (parallel to Urls), "" if none
        std::string              Body;          // request body (non-GET)
        std::string              ContentType;   // forwarded request Content-Type
        std::string              Accept;        // forwarded request Accept (asset content negotiation)
        LocalFallback            Fallback;      // run on the event loop if every layer misses
        ResponseTransform        Transform;     // applied to a winning layer's body before replying
    };

    // The decoded result of walking the layer stack, handed back to the event loop.
    struct ProxyResult {
        bool                       Served {false}; // a layer returned 2xx
        bool                       GotHttp {false};// at least one layer answered with any HTTP status
        long                       Status {0};     // winning status, or the last miss status
        std::string                ContentType;
        std::string                ContentDisposition;
        std::vector<unsigned char> Body;
    };

    void StartPool();
    void StopPool();
    void RunWorker();                                            // worker threads

    void OnComplete(std::shared_ptr<ProxyRequest> r, ProxyResult result); // event-loop thread
    static void OnClientDisconnect(evhttp_connection *conn, void *arg);

    ServerEmulator *mEmu;

    //////////////// Layer stack ////////////////
    mutable std::mutex mLayersMutex;
    std::vector<Layer> mLayers; // top-first

    //////////////// Forwarding thread pool ////////////////
    std::vector<std::thread>                  mThreads;
    std::mutex                                mQueueMutex; // guards mQueue
    std::condition_variable                   mCv;
    std::deque<std::shared_ptr<ProxyRequest>> mQueue;
    std::atomic<bool>                         mPoolRunning {false}; // worker threads alive?
    std::atomic<bool>                         mActive {true};       // false while the server is stopped

    // In-flight forwards. Kept so Pause can reach them before the server shuts down.
    // Only ever touched on the event-loop thread.
    std::unordered_set<std::shared_ptr<ProxyRequest>> mActiveRequests;

    static constexpr size_t kThreadCount = 6;
};
}
