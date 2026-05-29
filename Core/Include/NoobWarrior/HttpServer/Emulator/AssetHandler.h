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
// File: AssetHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace NoobWarrior {
class ServerEmulator;

// AssetHandler serves asset requests. If an asset isn't in the local database, it's fetched from
// Roblox. That fetch is slow, so instead of blocking the single HTTP thread it's handed to a pool
// of worker threads; the reply is sent once the fetch finishes.
//
// Threading rule of thumb: the worker threads ONLY do the network fetch. Everything that touches
// the HTTP request, the connection, or the database happens on the event-loop thread.
class AssetHandler : public Handler {
public:
    AssetHandler(ServerEmulator *srv, EmuDbManager *dbm);
    ~AssetHandler() override;
    void OnRequest(evhttp_request *req, void *userdata) override;

    // ServerEmulator calls these (on the event-loop thread) when the HTTP server stops/starts.
    // PauseProxy MUST run before the server's evhttp is freed: it makes any in-flight fetch give up
    // quietly instead of replying to a connection that's about to be destroyed. The worker threads
    // themselves keep running across restarts; they're only stopped when the handler is destroyed.
    void PauseProxy();
    void ResumeProxy();
private:
    // One asset being fetched from Roblox. The client's request stays open until the fetch finishes
    // and OnFetchComplete replies. Shared between the event-loop thread and one worker thread.
    struct ProxyFetch {
        evhttp_request*    Request    {nullptr};
        evhttp_connection* Connection {nullptr};
        std::atomic<bool>  ClientConnected {true}; // set false if the client hangs up mid-fetch

        int64_t         Id      {0};
        int             Version {0};
        std::string     Url;
        std::string     Cookie;
        SqlDb::Response MissResult {};        // reply code to use if the fetch fails
        bool            SaveToGrabDb {false};
        std::string     GrabDbPath;
    };

    void StartProxyPool();
    void StopProxyPool();
    void RunProxyWorker();                                       // worker threads

    // Everything below runs on the event-loop thread.
    void BeginProxyFetch(evhttp_request *req, int64_t id, int version, SqlDb::Response missResult);
    // Called on the event loop once a worker's network fetch finishes. The cpr result is decoded
    // into plain fields by the worker so this header doesn't depend on the HTTP client.
    void OnFetchComplete(std::shared_ptr<ProxyFetch> fetch, bool ok, long httpStatus, std::vector<unsigned char> data);
    void ReplyWithAsset(evhttp_request *req, SqlDb::Response res,
                        const std::vector<unsigned char> &data, const std::string &hash);
    void SaveGrabbedAsset(const std::string &dbFilePath, int64_t id, int version,
                          const std::vector<unsigned char> &data);
    static void OnClientDisconnect(evhttp_connection *conn, void *arg);

    ServerEmulator *mServerEmulator;
    EmuDbManager *mEmuDbManager;

    //////////////// Proxy thread pool ////////////////
    std::vector<std::thread>                mProxyThreads;
    std::mutex                              mProxyMutex;   // guards mProxyQueue
    std::condition_variable                 mProxyCv;
    std::deque<std::shared_ptr<ProxyFetch>> mProxyQueue;
    std::atomic<bool>                       mPoolRunning {false}; // worker threads alive?
    std::atomic<bool>                       mProxyActive {true};  // false while the server is stopped

    // Fetches currently in flight. Kept so PauseProxy can reach them before the server shuts down.
    // Only ever touched on the event-loop thread.
    std::unordered_set<std::shared_ptr<ProxyFetch>> mActiveFetches;

    static constexpr size_t kProxyThreadCount = 8;
};
}
