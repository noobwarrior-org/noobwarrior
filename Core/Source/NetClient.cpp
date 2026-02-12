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

static size_t WriteToBuf(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

static size_t WriteToDisk(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::ofstream*>(userp)->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

NetClient::NetClient(Account *account) : NetClient() {
    std::string cookies;
    if (mAccount != nullptr) {
        cookies.append(std::format(".ROBLOSECURITY={};", mAccount->Token));
        curl_easy_setopt(mHandle, CURLOPT_USERAGENT, "Roblox/WinINet");
    }

    curl_easy_setopt(mHandle, CURLOPT_COOKIE, cookies.c_str());
}

NetClient::NetClient() : mFailReason(FailReason::Unknown) {
    mHandle = curl_easy_init();
    if (mHandle == nullptr)
        return;

    mHeaderList = curl_slist_append(NULL, "suck my dick");

    curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteToBuf);
    curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &mData);
    
    mFailReason = FailReason::None;
}

NetClient::~NetClient() {
    curl_slist_free_all(mHeaderList);
    curl_easy_cleanup(mHandle);
}

bool NetClient::Fail() {
    return mFailReason != FailReason::None;
}

CURLcode NetClient::Request(const std::string &url) {
    curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
    
    return curl_easy_perform(mHandle);
}

void NetClient::OnWriteToMemoryFinished(std::function<void(std::vector<unsigned char>&)> callback) {
    callback(mData);
}

void NetClient::SetHeader(const std::string &name, const std::string &contents) {
    mHeaderList = curl_slist_append(mHeaderList, std::string(name + ": " + contents).c_str());
    curl_easy_setopt(mHandle, CURLOPT_HTTPHEADER, mHeaderList);
}

void NetClient::SetUserAgent(const std::string &str) {
    SetHeader("User-Agent", str);
}