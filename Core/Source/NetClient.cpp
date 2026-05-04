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
// File: NetClient.cpp
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A thin layer over libcurl that makes it less annoying to use
#include <NoobWarrior/NetClient.h>

using namespace NoobWarrior;

size_t NetClient::WriteToBuf(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    auto* buf = static_cast<std::vector<unsigned char>*>(userp);
    const auto* begin = static_cast<unsigned char*>(contents);
    buf->insert(buf->end(), begin, begin + total);
    return total;
}

size_t NetClient::WriteToDisk(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    static_cast<std::ofstream*>(userp)->write(static_cast<char*>(contents), static_cast<std::streamsize>(total));
    return total;
}

int NetClient::OnProgress(void* userp,
                          curl_off_t dlTotal, curl_off_t dlNow,
                          curl_off_t ulTotal, curl_off_t ulNow) {
    auto* self = static_cast<NetClient*>(userp);
    const DownloadProgress prog { dlTotal, dlNow, ulTotal, ulNow };
    for (auto& cb : self->mProgressCallbacks)
        cb(prog);
    return 0; // Non-zero aborts the transfer.
}

NetClient::NetClient(Account *account) : NetClient() {
    mAccount = account;

    if (mAccount) {
        const std::string cookie = std::format(".ROBLOSECURITY={};", mAccount->Token);
        curl_easy_setopt(mHandle, CURLOPT_COOKIE,    cookie.c_str());
        curl_easy_setopt(mHandle, CURLOPT_USERAGENT, "Roblox/WinINet");
    }
}

NetClient::NetClient() : mFailReason(FailReason::Unknown), mHeaderList(nullptr) {
    mHandle = curl_easy_init();
    if (mHandle == nullptr) {
        mFailReason = FailReason::CurlInitFailed;
        return;
    }

    curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteToBuf);
    curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &mData);

    curl_easy_setopt(mHandle, CURLOPT_XFERINFOFUNCTION, OnProgress);
    curl_easy_setopt(mHandle, CURLOPT_XFERINFODATA,     this);
    curl_easy_setopt(mHandle, CURLOPT_NOPROGRESS,       0L);

    curl_easy_setopt(mHandle, CURLOPT_FOLLOWLOCATION, 1L);
    
    mFailReason = FailReason::None;
}

NetClient::~NetClient() {
    if (mHeaderList != nullptr)
        curl_slist_free_all(mHeaderList);
    curl_easy_cleanup(mHandle);
}

bool NetClient::Fail() {
    return mFailReason != FailReason::None;
}

void NetClient::AddToQueueAsync(const Url &url) {
    mPendingDownloads.push_back(url);
}

void NetClient::StartDownloadAsync(const DownloadOptions &options) {
    for (auto &url : mPendingDownloads) {
        auto sharedData = std::make_shared<std::vector<unsigned char>>();
        auto sharedCallbacks = std::make_shared<std::vector<std::function<void(const std::vector<unsigned char>&)>>>(mMemoryCallbacks);

        std::thread thread([sharedData, sharedCallbacks, options, url, this]() -> void {
            Transfer transfer;
            CURL* handle = curl_easy_init();

            curl_easy_setopt(handle, CURLOPT_URL, url.Resolve().c_str());
            curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

            if (options.OutputFormat == DownloadOutputFormat::Memory) {
                curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteToBuf);
                curl_easy_setopt(handle, CURLOPT_WRITEDATA, sharedData.get());
            }

            CURLcode res = curl_easy_perform(handle);
            curl_easy_cleanup(handle);

            if (options.OutputFormat == DownloadOutputFormat::Memory) {
                for (auto& callback : *sharedCallbacks) {
                    callback(*sharedData);
                }
            }
        });
        thread.detach();
    }
}

CURLcode NetClient::RequestSync(const std::string &url) {
    mData.clear();
    curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteToBuf);
    curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &mData);
    CURLcode code = curl_easy_perform(mHandle);
    for (auto& callback : mMemoryCallbacks) {
        callback(mData);
    }
    return code;
}

long NetClient::GetHttpCodeSync() const {
    long http_code = 0;
    curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &http_code);
    return http_code;
}

void NetClient::SetTimeoutSync(long timeout) {
    curl_easy_setopt(mHandle, CURLOPT_TIMEOUT, timeout);
}

void NetClient::OnDownloadProgress(std::function<void(const DownloadProgress&)> callback) {
    mProgressCallbacks.push_back(std::move(callback));
}

void NetClient::OnWriteToMemoryFinished(std::function<void(const std::vector<unsigned char>&)> callback) {
    mMemoryCallbacks.push_back(std::move(callback));
}

void NetClient::OnFileDownloaded(std::function<void(const std::filesystem::path&)> callback) {
    mFileCallbacks.push_back(std::move(callback));
}

void NetClient::SetHeader(const std::string &name, const std::string &value) {
    curl_slist_free_all(mHeaderList);

    mHeaderList = curl_slist_append(mHeaderList, std::string(name + ": " + value).c_str());
    curl_easy_setopt(mHandle, CURLOPT_HTTPHEADER, mHeaderList);
}

void NetClient::SetUserAgent(const std::string &userAgent) {
    curl_easy_setopt(mHandle, CURLOPT_USERAGENT, userAgent.c_str());
}

std::vector<unsigned char> NetClient::GetData() {
    return mData;
}
