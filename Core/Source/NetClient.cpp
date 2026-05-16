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
// Description: Thin wrapper over libcurl. Fetch one URL or many in parallel.
#include <NoobWarrior/NetClient.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <thread>

using namespace NoobWarrior;

static size_t WriteToBuf(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t total = size * nmemb;
    auto *buf = static_cast<std::vector<unsigned char>*>(userp);
    const auto *bytes = static_cast<const unsigned char*>(contents);
    buf->insert(buf->end(), bytes, bytes + total);
    return total;
}

struct PreparedHandle {
    CURL *Handle = nullptr;
    curl_slist *HeaderList = nullptr;
    HttpResponse Response;

    ~PreparedHandle() {
        if (HeaderList) curl_slist_free_all(HeaderList);
        if (Handle)     curl_easy_cleanup(Handle);
    }
    PreparedHandle() = default;
    PreparedHandle(PreparedHandle&&) = delete;
    PreparedHandle& operator=(PreparedHandle&&) = delete;
    PreparedHandle(const PreparedHandle&) = delete;
    PreparedHandle& operator=(const PreparedHandle&) = delete;
};

static bool ConfigureHandle(PreparedHandle &slot, const HttpRequest &req) {
    slot.Handle = curl_easy_init();
    if (!slot.Handle) return false;

    curl_easy_setopt(slot.Handle, CURLOPT_URL, req.Url.c_str());
    curl_easy_setopt(slot.Handle, CURLOPT_FOLLOWLOCATION, req.FollowRedirects ? 1L : 0L);
    curl_easy_setopt(slot.Handle, CURLOPT_TIMEOUT, req.TimeoutSeconds);

    curl_easy_setopt(slot.Handle, CURLOPT_WRITEFUNCTION, WriteToBuf);
    curl_easy_setopt(slot.Handle, CURLOPT_WRITEDATA, &slot.Response.Body);

    if (req.IgnoreTLSVerification) {
        curl_easy_setopt(slot.Handle, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(slot.Handle, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    curl_easy_setopt(slot.Handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

    if (!req.Cookie.empty())
        curl_easy_setopt(slot.Handle, CURLOPT_COOKIE, req.Cookie.c_str());
    if (!req.UserAgent.empty())
        curl_easy_setopt(slot.Handle, CURLOPT_USERAGENT, req.UserAgent.c_str());

    for (const auto &[name, value] : req.Headers) {
        std::string line = name + ": " + value;
        slot.HeaderList = curl_slist_append(slot.HeaderList, line.c_str());
    }
    if (slot.HeaderList)
        curl_easy_setopt(slot.Handle, CURLOPT_HTTPHEADER, slot.HeaderList);

    return true;
}

NetClient::NetClient() = default;
NetClient::~NetClient() = default;

HttpResponse NetClient::Fetch(const HttpRequest &request) {
    PreparedHandle slot;
    if (!ConfigureHandle(slot, request)) {
        slot.Response.Code = CURLE_FAILED_INIT;
        return std::move(slot.Response);
    }

    slot.Response.Code = curl_easy_perform(slot.Handle);
    if (slot.Response.Code == CURLE_OK)
        curl_easy_getinfo(slot.Handle, CURLINFO_RESPONSE_CODE, &slot.Response.HttpStatus);

    return std::move(slot.Response);
}

std::vector<HttpResponse> NetClient::FetchMany(const std::vector<HttpRequest> &requests) {
    std::vector<HttpResponse> results(requests.size());
    if (requests.empty()) return results;

    const size_t kMaxConcurrency = 8;
    size_t hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;
    size_t workers = std::min({requests.size(), kMaxConcurrency, hw});

    std::atomic<size_t> nextIndex {0};
    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (size_t w = 0; w < workers; w++) {
        threads.emplace_back([&]() {
            for (;;) {
                size_t i = nextIndex.fetch_add(1, std::memory_order_relaxed);
                if (i >= requests.size()) return;
                results[i] = Fetch(requests[i]);
            }
        });
    }

    for (auto &t : threads) t.join();
    return results;
}
