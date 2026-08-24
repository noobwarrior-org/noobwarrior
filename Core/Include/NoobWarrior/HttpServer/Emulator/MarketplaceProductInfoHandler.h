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
// File: MarketplaceProductInfoHandler.h
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves MarketplaceService asset product info from EmuDb, remote emulators, or Roblox.
#pragma once
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/HttpServer/Base/Handler.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
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

class MarketplaceProductInfoHandler : public Handler {
public:
    MarketplaceProductInfoHandler(ServerEmulator *server, EmuDbManager *dbm);
    ~MarketplaceProductInfoHandler() override;
    void OnRequest(evhttp_request *req, void *userdata) override;

    // The Roblox fallback runs off the event loop. ServerEmulator pauses it before evhttp is freed,
    // matching AssetHandler's request-lifetime rules.
    void PauseProxy();
    void ResumeProxy();
private:
    struct ProxyFetch {
        evhttp_request *Request {nullptr};
        evhttp_connection *Connection {nullptr};
        std::atomic<bool> ClientConnected {true};

        int64_t AssetId {0};
        std::string Url;
        std::string Cookie;
    };

    void StartProxyPool();
    void StopProxyPool();
    void RunProxyWorker();
    void BeginRobloxFetch(evhttp_request *req, int64_t assetId);
    void OnFetchComplete(std::shared_ptr<ProxyFetch> fetch, bool ok, long httpStatus, std::string body);
    static void OnClientDisconnect(evhttp_connection *connection, void *userdata);

    ServerEmulator *mServerEmulator;
    EmuDbManager *mEmuDbManager;

    std::vector<std::thread> mProxyThreads;
    std::mutex mProxyMutex;
    std::condition_variable mProxyCv;
    std::deque<std::shared_ptr<ProxyFetch>> mProxyQueue;
    std::atomic<bool> mPoolRunning {false};
    std::atomic<bool> mProxyActive {true};
    std::unordered_set<std::shared_ptr<ProxyFetch>> mActiveFetches;

    static constexpr size_t kProxyThreadCount = 2;
    static constexpr int kProxyMaxAttempts = 3;
};
}
