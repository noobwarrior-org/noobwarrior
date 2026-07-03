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
// File: EmulatorProxy.cpp
// Started by: Hattozo
// Started on: 6/20/2026
// Description: Layered reverse proxy to remote server emulators. See EmulatorProxy.h.
//              Some assistance by Claude Opus 4.8
#include <cpr/cpr.h>

#include <NoobWarrior/HttpServer/Emulator/EmulatorProxy.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Log.h>

#include <algorithm>

using namespace NoobWarrior;

EmulatorProxy::EmulatorProxy(ServerEmulator *emu) : mEmu(emu) {
    StartPool();
}

EmulatorProxy::~EmulatorProxy() {
    StopPool();
}

void EmulatorProxy::PushLayer(const std::string &host, uint16_t port, const std::string &sessionToken) {
    std::lock_guard lock(mLayersMutex);
    // a layer is identified by host + port; the session token is per-connection metadata. If it's
    // already present, lift it to the top instead of duplicating it.
    auto matches = [&](const Layer &l) { return l.Host == host && l.Port == port; };
    auto it = std::find_if(mLayers.begin(), mLayers.end(), matches);
    if (it != mLayers.end())
        mLayers.erase(it);
    mLayers.insert(mLayers.begin(), Layer{host, port, sessionToken}); // newest layer is tried first
    Out("EmulatorProxy", "Pushed proxy layer {}:{} (depth now {}{})", host, port, mLayers.size(),
        sessionToken.empty() ? "" : ", authenticated");
}

bool EmulatorProxy::PopLayer() {
    std::lock_guard lock(mLayersMutex);
    if (mLayers.empty())
        return false;
    Out("EmulatorProxy", "Popped proxy layer {}:{}", mLayers.front().Host, mLayers.front().Port);
    mLayers.erase(mLayers.begin());
    return true;
}

void EmulatorProxy::RemoveLayer(const std::string &host, uint16_t port) {
    std::lock_guard lock(mLayersMutex);
    auto it = std::find_if(mLayers.begin(), mLayers.end(),
                           [&](const Layer &l) { return l.Host == host && l.Port == port; });
    if (it != mLayers.end()) {
        mLayers.erase(it);
        Out("EmulatorProxy", "Removed proxy layer {}:{} (depth now {})", host, port, mLayers.size());
    }
}

void EmulatorProxy::ClearLayers() {
    std::lock_guard lock(mLayersMutex);
    if (!mLayers.empty())
        Out("EmulatorProxy", "Cleared all {} proxy layer(s)", mLayers.size());
    mLayers.clear();
}

bool EmulatorProxy::HasLayers() const {
    std::lock_guard lock(mLayersMutex);
    return !mLayers.empty();
}

std::vector<EmulatorProxy::Layer> EmulatorProxy::GetLayers() const {
    std::lock_guard lock(mLayersMutex);
    return mLayers;
}

void EmulatorProxy::StartPool() {
    bool expected = false;
    if (!mPoolRunning.compare_exchange_strong(expected, true))
        return;
    for (size_t i = 0; i < kThreadCount; i++)
        mThreads.emplace_back(&EmulatorProxy::RunWorker, this);
}

void EmulatorProxy::StopPool() {
    if (!mPoolRunning.exchange(false))
        return;
    mCv.notify_all();
    for (auto &t : mThreads)
        if (t.joinable())
            t.join();
    mThreads.clear();
}

void EmulatorProxy::Pause() {
    // The server is about to free its evhttp, so any forward still running must not try to reply.
    // Mark replies off and detach every in-flight request from its (soon-to-be-freed) connection.
    // The worker threads keep their own shared_ptr to each ProxyRequest, so this stays safe.
    mActive = false;
    for (auto &r : mActiveRequests) {
        if (r->Connection)
            evhttp_connection_set_closecb(r->Connection, nullptr, nullptr);
        r->ClientConnected = false;
    }
    mActiveRequests.clear();
}

void EmulatorProxy::Resume() {
    mActive = true;
}

void EmulatorProxy::OnClientDisconnect(evhttp_connection *conn, void *arg) {
    // The client hung up before the forward finished. Just note it; OnComplete will see the flag
    // and skip the reply. (arg stays valid: mActiveRequests holds the ProxyRequest until then.)
    static_cast<ProxyRequest*>(arg)->ClientConnected = false;
}

bool EmulatorProxy::TryProxy(evhttp_request *req, LocalFallback localFallback, ResponseTransform transform) {
    if (!mActive)
        return false;

    std::vector<Layer> layers = GetLayers();
    if (layers.empty())
        return false; // nothing overlaid; caller handles the request locally

    const char *uri = evhttp_request_get_uri(req);
    const std::string path = uri ? uri : "/";

    auto r = std::make_shared<ProxyRequest>();
    r->Request    = req;
    r->Connection = evhttp_request_get_connection(req);
    r->Fallback   = std::move(localFallback);
    r->Transform  = std::move(transform);
    r->Urls.reserve(layers.size());
    r->Cookies.reserve(layers.size());
    for (const Layer &layer : layers) {
        r->Urls.push_back("https://" + layer.Host + ":" + std::to_string(layer.Port) + path);
        r->Cookies.push_back(layer.SessionToken); // forwarded so the host can identify the joiner
    }

    switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_POST: r->Method = "POST"; break;
    case EVHTTP_REQ_PUT:  r->Method = "PUT";  break;
    case EVHTTP_REQ_GET:
    default:              r->Method = "GET";  break;
    }

    // Body only matters for the methods that carry one (join-game is a POST with a JSON body).
    // copyout is non-destructive, so the local fallback can still read it later.
    if (r->Method != "GET") {
        if (evbuffer *buf = evhttp_request_get_input_buffer(req)) {
            size_t len = evbuffer_get_length(buf);
            if (len > 0) {
                r->Body.resize(len);
                evbuffer_copyout(buf, r->Body.data(), len);
            }
        }
    }

    evkeyvalq *inHeaders = evhttp_request_get_input_headers(req);
    if (const char *accept = evhttp_find_header(inHeaders, "Accept"))
        r->Accept = accept;
    if (const char *ct = evhttp_find_header(inHeaders, "Content-Type"))
        r->ContentType = ct;

    // Get told if the client disconnects before the forward finishes.
    if (r->Connection)
        evhttp_connection_set_closecb(r->Connection, &EmulatorProxy::OnClientDisconnect, r.get());

    mActiveRequests.insert(r);

    {
        std::lock_guard lock(mQueueMutex);
        mQueue.push_back(r);
    }
    mCv.notify_one();
    return true;
}

void EmulatorProxy::RunWorker() {
    // One session per worker, reused across forwards. Remote emulators almost always present a
    // self-signed certificate, so verification is disabled (same as Core::ConnectToServerEmulator).
    cpr::Session session;
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
    session.SetTimeout(cpr::Timeout{20000});             // 20s per attempt
    session.SetConnectTimeout(cpr::ConnectTimeout{10000});
    session.SetVerifySsl(cpr::VerifySsl{false});

    while (mPoolRunning) {
        std::shared_ptr<ProxyRequest> r;
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mCv.wait(lock, [&] { return !mPoolRunning || !mQueue.empty(); });
            if (!mPoolRunning)
                break;
            r = std::move(mQueue.front());
            mQueue.pop_front();
        }

        // The method and body are identical for every layer; only the host (and its session cookie)
        // change. SetHeader/SetBody replace, so nothing leaks from the previous forward.
        cpr::Header baseHeaders;
        if (!r->Accept.empty())      baseHeaders["Accept"]       = r->Accept;
        if (!r->ContentType.empty()) baseHeaders["Content-Type"] = r->ContentType;
        session.SetBody(cpr::Body{r->Body});

        ProxyResult result;
        // Walk the stack top -> bottom. First 2xx wins; a non-2xx answer or an unreachable layer
        // falls through to the next one down.
        for (size_t i = 0; i < r->Urls.size(); ++i) {
            if (!mPoolRunning)
                break;

            // Attach this layer's session cookie so the host can authenticate the joiner.
            cpr::Header headers = baseHeaders;
            if (i < r->Cookies.size() && !r->Cookies[i].empty())
                headers["Cookie"] = ".LOGINSESSION=" + r->Cookies[i];
            session.SetHeader(headers);
            session.SetUrl(cpr::Url{r->Urls[i]});

            cpr::Response resp = (r->Method == "GET") ? session.Get()
                               : (r->Method == "PUT") ? session.Put()
                                                      : session.Post();
            if (resp.error.code != cpr::ErrorCode::OK)
                continue; // layer unreachable -> try the one below

            // This layer answered. Remember it as the most recent answer in case every layer misses
            // and there's no local fallback, so the client still gets a sensible (e.g. 404) reply.
            result.GotHttp = true;
            result.Status  = resp.status_code;
            result.Body.assign(resp.text.begin(), resp.text.end());
            result.ContentType.clear();
            result.ContentDisposition.clear();
            if (auto it = resp.header.find("Content-Type"); it != resp.header.end())
                result.ContentType = it->second;
            if (auto it = resp.header.find("Content-Disposition"); it != resp.header.end())
                result.ContentDisposition = it->second;

            if (resp.status_code >= 200 && resp.status_code < 300) {
                result.Served = true;
                break; // this layer has it
            }
            // non-2xx (e.g. 404 "doesn't have it") -> fall through to the next layer
        }

        // The forward is done; the reply (or local fallback) happens back on the event loop.
        mEmu->GetCore()->RunOnEventLoop(
            [this, r, result = std::move(result)]() mutable {
                OnComplete(r, std::move(result));
            });
    }
}

void EmulatorProxy::OnComplete(std::shared_ptr<ProxyRequest> r, ProxyResult result) {
    mActiveRequests.erase(r); // this forward is finished, one way or another

    if (!mActive)            return; // server stopped; don't touch evhttp
    if (!r->ClientConnected) return; // client hung up; its request is already gone

    // Done with the disconnect notification now that we're about to hand off / reply.
    if (r->Connection)
        evhttp_connection_set_closecb(r->Connection, nullptr, nullptr);

    auto reply = [&](int status, const std::string &contentType, const std::string &contentDisposition,
                     const std::vector<unsigned char> &body) {
        evkeyvalq *outHeaders = evhttp_request_get_output_headers(r->Request);
        if (!contentType.empty())
            evhttp_add_header(outHeaders, "Content-Type", contentType.c_str());
        if (!contentDisposition.empty())
            evhttp_add_header(outHeaders, "Content-Disposition", contentDisposition.c_str());

        evbuffer *buf = evbuffer_new();
        if (!body.empty())
            evbuffer_add(buf, body.data(), body.size());
        // libevent fills in a default reason phrase when passed nullptr for a known status code.
        evhttp_send_reply(r->Request, status != 0 ? status : 200, nullptr, buf);
        evbuffer_free(buf);
    };

    if (result.Served) {
        // Let the handler overlay client-only data onto the proxied body (e.g. local player identity
        // onto a host-authored join script) before it goes out.
        if (r->Transform)
            result.Body = r->Transform(std::move(result.Body));
        reply(static_cast<int>(result.Status), result.ContentType, result.ContentDisposition, result.Body);
        return;
    }

    // No layer had it. Fall through to the caller's local handling (the implicit bottom layer).
    if (r->Fallback) {
        r->Fallback(r->Request);
        return;
    }

    // No local fallback: forward the last layer's miss verbatim, or a gateway error if every layer
    // was unreachable.
    if (result.GotHttp)
        reply(static_cast<int>(result.Status), result.ContentType, result.ContentDisposition, result.Body);
    else
        evhttp_send_error(r->Request, 502, "All proxy layers unreachable");
}
