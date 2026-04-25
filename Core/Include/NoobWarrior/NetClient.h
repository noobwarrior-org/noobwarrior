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
// File: NetClient.h
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A thin layer over libcurl that makes it less annoying to use
#pragma once
#include <NoobWarrior/Url.h>
#include <NoobWarrior/Keychain/RbxKeychain.h>

#include <curl/curl.h>

#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace NoobWarrior {
struct DownloadProgress {
    curl_off_t DownloadTotal;
    curl_off_t DownloadNow;
    curl_off_t UploadTotal;
    curl_off_t UploadNow;
};

// Represents an in-flight async download.
// Canceled can be set from any thread to abort the transfer.
struct Transfer {
    CURL* Handle = nullptr;
    std::shared_ptr<std::atomic_bool> Canceled = std::make_shared<std::atomic_bool>(false);
    std::shared_ptr<std::ofstream> File;
    std::vector<unsigned char> Data;
    DownloadProgress Progress {};
};

enum class DownloadOutputFormat {
    Memory,
    FileSystem
};

struct DownloadOptions {
    DownloadOutputFormat OutputFormat { DownloadOutputFormat::Memory };
    std::filesystem::path OutputDir {};
};

class NetClient {
public:
    enum class FailReason {
        None,
        Unknown,
        CurlInitFailed
    };

    NetClient(Account *account);
    NetClient();
    ~NetClient();

    bool Fail();

    /* these functions do not block and are multi-threaded under the hood. you can safely use them on the main thread. */
    void AddToQueueAsync(const Url &url);
    void StartDownloadAsync(const DownloadOptions &options = {});

    /* these functions are simple wrappers over the curl_easy functions and are blocking */
    CURLcode RequestSync(const std::string &url);
    long GetHttpCodeSync() const;
    void SetTimeoutSync(long timeout);

    void OnDownloadProgress(std::function<void(const DownloadProgress&)> callback);
    void OnWriteToMemoryFinished(std::function<void(const std::vector<unsigned char>&)> callback);
    void OnFileDownloaded(std::function<void(const std::filesystem::path&)> callback);
    
    void SetHeader(const std::string &name, const std::string &value);
    void SetUserAgent(const std::string &userAgent);

    std::vector<unsigned char> GetData();
private:
    FailReason mFailReason;
    std::vector<unsigned char> mData;

    std::vector<Url> mPendingDownloads;
    std::vector<Transfer> mTransfers;

    std::vector<std::function<void(const DownloadProgress&)>>         mProgressCallbacks;
    std::vector<std::function<void(const std::vector<unsigned char>&)>> mMemoryCallbacks;
    std::vector<std::function<void(const std::filesystem::path&)>>     mFileCallbacks;

    struct curl_slist *mHeaderList;
    std::vector<std::string> mHeaders;
    Account *mAccount;
    CURL *mHandle;

    // curl write/progress callbacks.
    static size_t WriteToBuf(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t WriteToDisk(void* contents, size_t size, size_t nmemb, void* userp);
    static int    OnProgress(void* userp, curl_off_t dlTotal, curl_off_t dlNow,
                             curl_off_t ulTotal, curl_off_t ulNow);
};
}