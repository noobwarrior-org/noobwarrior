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
#include <NoobWarrior/Auth/RobloxAuth.h>

#include <curl/curl.h>

#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace NoobWarrior {
struct Transfer {
    CURL* Handle = nullptr;
    std::shared_ptr<std::atomic_bool> Cancelled = std::make_shared<std::atomic_bool>(false);
    std::shared_ptr<std::ofstream> File;
    size_t DownloadTotal {};
    size_t DownloadNow {};
    size_t UploadTotal {};
    size_t UploadNow {};
};

enum class DownloadOutputFormat {
    Memory,
    FileSystem
};

struct DownloadOptions {
    DownloadOutputFormat OutputFormat;
    std::filesystem::path OutputDir;
};

class NetClient {
public:
    enum class FailReason {
        None,
        Unknown
    };

    NetClient(RobloxAccount *account);
    NetClient();
    ~NetClient();

    bool Fail();

    void AddToQueue(const Url &url);
    void StartDownload(const DownloadOptions &options);

    CURLcode Request(const std::string &url);

    void OnDownloadProgress();
    void OnWriteToMemoryFinished(std::function<void(std::vector<unsigned char>&)> callback);
    void OnFileDownloaded(std::function<void()> callback);
    
    void SetHeader(const std::string &name, const std::string &contents);
    void SetUserAgent(const std::string &str);
private:
    FailReason mFailReason;
    std::vector<unsigned char> mData;

    std::vector<std::thread> mDownloadThreads;

    struct curl_slist *mHeaderList;
    RobloxAccount *mRobloxAccount;
    CURL *mHandle;
};
}