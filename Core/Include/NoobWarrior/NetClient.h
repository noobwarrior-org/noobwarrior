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
// Description: A thin wrapper over libcurl: build an HttpRequest, get an HttpResponse.
//              FetchMany() runs N requests in parallel using curl_multi.
#pragma once
#include <curl/curl.h>

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace NoobWarrior {
struct HttpRequest {
    std::string Url;
    std::string Method { "GET" };
    std::string PostBody;
    std::string ContentType;
    std::vector<std::pair<std::string, std::string>> Headers;
    std::string Cookie;          // raw cookie header value, e.g. ".ROBLOSECURITY=xxx;"
    std::string UserAgent;
    long TimeoutSeconds { 30 };
    bool FollowRedirects { true };
    bool IgnoreTLSVerification { false };
};

struct HttpResponse {
    CURLcode Code { CURLE_OK };    // network-level result
    long HttpStatus { 0 };         // HTTP response code, 0 if Code != CURLE_OK
    std::vector<unsigned char> Body;
};

class NetClient {
public:
    NetClient();
    ~NetClient();
    HttpResponse Fetch(const HttpRequest &request);
    std::vector<HttpResponse> FetchMany(const std::vector<HttpRequest> &requests);
};
}
