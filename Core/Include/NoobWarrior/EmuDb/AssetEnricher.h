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
// File: AssetEnricher.h
// Started by: Hattozo
// Started on: 5/28/2026
// Description: (Disclaimer: This code was created by Claude Opus 4.8 and Fable 5.)
//              Background worker that fills in metadata (name/description/dates/type/creator)
//              and thumbnail images for assets captured by assetGrabMode.
//
//              The grab hot path saves an asset + its data blob immediately with a placeholder
//              name, then hands the id to this enricher. Roblox's per-asset details endpoint
//              (economy /v2/assets/{id}/details) is rate-limited to ~1 request/minute, so this
//              instead uses batch endpoints off the request thread:
//                - develop.roblox.com/v1/assets?assetIds=...   (metadata, 100 req/min, needs auth)
//                - thumbnails.roblox.com/v1/assets?assetIds=... (images, 50 req/sec, no auth)
//
//              To stay clear of the GUI/event-loop thread that owns the mounted SQLite
//              connection, the worker opens its OWN EmuDb connection per target file. SQLite's
//              file locking coordinates the two single-threaded connections (the mounted DBs are
//              opened with autocommit, so writes commit and become visible promptly).
#pragma once
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <set>
#include <utility>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace NoobWarrior {
class Core;
class EmuDb;

Roblox::AssetType AssetTypeFromApiString(const std::string &s);
int64_t ParseIso8601Utc(const std::string &s);

class AssetEnricher {
public:
    explicit AssetEnricher(Core *core);
    ~AssetEnricher();

    void Start();
    void Stop();

    // Queue an asset (located in the given .nwdb file) for background enrichment.
    // Safe to call from the HTTP handler / event-loop thread.
    void Enqueue(const std::string &dbFilePath, int64_t assetId);

private:
    struct Job {
        std::string DbFilePath;
        int64_t     AssetId {0};
        std::string Cookie;   // snapshotted from the keychain on the enqueueing thread
        int         Attempts {0};
    };

    enum class BatchResult {
        Ok,         // processed (even if some assets were missing from the response)
        RetryLater, // transient failure (network, rate limit, connection): re-queue the jobs
    };

    void WorkerLoop();
    BatchResult ProcessBatch(const std::string &dbFilePath, const std::vector<Job> &jobs);

    // Worker-thread-owned connections, keyed by file path. Never touched by other threads.
    // Failures are NOT cached: a database whose Mutable flag gets turned back on (or whose open
    // lost a transient lock race) starts working on the next batch instead of after a restart.
    EmuDb* GetConnection(const std::string &dbFilePath);
    void CloseConnections();

    Core *mCore;

    std::thread             mThread;
    std::atomic<bool>       mRunning {false};

    std::mutex              mMutex;
    std::condition_variable mCv;
    std::deque<Job>         mQueue;
    // Dedupes (database, asset) pairs that are queued or in flight. Keyed on the pair, because
    // the same asset id can legitimately be destined for two different databases.
    std::set<std::pair<std::string, int64_t>> mPending;

    std::map<std::string, EmuDb*> mConnections;
    std::set<std::string>         mWarnedImmutable; // one refusal log per database

    static constexpr size_t kMaxBatchSize = 50;     // Roblox batch endpoints cap around here
    static constexpr int    kInterBatchDelayMs = 700; // keep develop calls well under 100/min
    static constexpr int    kMaxJobAttempts = 4;    // batches that keep failing get dropped
};
}
