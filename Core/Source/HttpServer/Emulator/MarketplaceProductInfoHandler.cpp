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
// File: MarketplaceProductInfoHandler.cpp
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves legacy /marketplace/productinfo and GetProductInfoV2 asset details from EmuDb,
//              with non-blocking fallbacks to remote emulators and the configured Roblox API.
#include <cpr/cpr.h>

#include <NoobWarrior/HttpServer/Emulator/MarketplaceProductInfoHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Url.h>

#include <nlohmann/json.hpp>

#include <curl/curl.h>

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <format>
#include <string>
#include <thread>
#include <utility>

using namespace NoobWarrior;

namespace {
std::string GetQueryParam(const char *uri, const char *key) {
    std::string value;
    if (uri == nullptr) return value;

    evkeyvalq query;
    if (evhttp_parse_query(uri, &query) == 0) {
        if (const char *found = evhttp_find_header(&query, key))
            value = found;
        evhttp_clear_headers(&query);
    }
    return value;
}

std::string UnixToIso(int64_t timestamp) {
    if (timestamp <= 0) return "2015-01-01T00:00:00Z";

    const std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm utc {};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif
    char value[32];
    std::strftime(value, sizeof(value), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return value;
}

void SendJson(evhttp_request *req, int status, const nlohmann::json &json) {
    const std::string body = json.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *buffer = evbuffer_new();
    evbuffer_add(buffer, body.data(), body.size());
    evhttp_send_reply(req, status, nullptr, buffer);
    evbuffer_free(buffer);
}

void SendMissing(evhttp_request *req) {
    SendJson(req, 404, {{"errors", nlohmann::json::array({
        {{"code", 0}, {"message", "Asset not found"}}
    })}});
}
}

MarketplaceProductInfoHandler::MarketplaceProductInfoHandler(ServerEmulator *server, EmuDbManager *dbm) :
    mServerEmulator(server),
    mEmuDbManager(dbm)
{
    StartProxyPool();
}

MarketplaceProductInfoHandler::~MarketplaceProductInfoHandler() {
    StopProxyPool();
}

void MarketplaceProductInfoHandler::StartProxyPool() {
    bool expected = false;
    if (!mPoolRunning.compare_exchange_strong(expected, true))
        return;
    for (size_t i = 0; i < kProxyThreadCount; i++)
        mProxyThreads.emplace_back(&MarketplaceProductInfoHandler::RunProxyWorker, this);
}

void MarketplaceProductInfoHandler::StopProxyPool() {
    if (!mPoolRunning.exchange(false))
        return;
    mProxyCv.notify_all();
    for (std::thread &thread : mProxyThreads)
        if (thread.joinable())
            thread.join();
    mProxyThreads.clear();
}

void MarketplaceProductInfoHandler::PauseProxy() {
    mProxyActive = false;
    for (const std::shared_ptr<ProxyFetch> &fetch : mActiveFetches) {
        if (fetch->Connection)
            evhttp_connection_set_closecb(fetch->Connection, nullptr, nullptr);
        fetch->ClientConnected = false;
    }
    mActiveFetches.clear();
}

void MarketplaceProductInfoHandler::ResumeProxy() {
    mProxyActive = true;
}

void MarketplaceProductInfoHandler::OnClientDisconnect(evhttp_connection *connection, void *userdata) {
    static_cast<ProxyFetch*>(userdata)->ClientConnected = false;
}

void MarketplaceProductInfoHandler::RunProxyWorker() {
    cpr::Session session;
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
    session.SetTimeout(cpr::Timeout{20000});
    session.SetConnectTimeout(cpr::ConnectTimeout{10000});
    curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);

    while (mPoolRunning) {
        std::shared_ptr<ProxyFetch> fetch;
        {
            std::unique_lock<std::mutex> lock(mProxyMutex);
            mProxyCv.wait(lock, [&] { return !mPoolRunning || !mProxyQueue.empty(); });
            if (!mPoolRunning)
                break;
            fetch = std::move(mProxyQueue.front());
            mProxyQueue.pop_front();
        }

        session.SetUrl(cpr::Url{fetch->Url});
        cpr::Header headers {{"Accept", "application/json"}};
        if (!fetch->Cookie.empty())
            headers["Cookie"] = fetch->Cookie;
        session.SetHeader(headers);

        bool ok = false;
        long httpStatus = 0;
        std::string body;
        for (int attempt = 0; attempt < kProxyMaxAttempts && mPoolRunning; attempt++) {
            cpr::Response response = session.Get();
            ok = response.error.code == cpr::ErrorCode::OK;
            httpStatus = response.status_code;

            const bool transient = !ok || httpStatus == 429 || httpStatus == 500
                || httpStatus == 502 || httpStatus == 503 || httpStatus == 504;
            if (!transient || attempt == kProxyMaxAttempts - 1) {
                body = std::move(response.text);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150 * (attempt + 1)));
        }

        mServerEmulator->GetCore()->RunOnEventLoop(
            [this, fetch, ok, httpStatus, body = std::move(body)]() mutable {
                OnFetchComplete(fetch, ok, httpStatus, std::move(body));
            });
    }
}

void MarketplaceProductInfoHandler::OnFetchComplete(std::shared_ptr<ProxyFetch> fetch, bool ok,
                                                     long httpStatus, std::string body) {
    mActiveFetches.erase(fetch);

    if (!mProxyActive || !fetch->ClientConnected)
        return;
    if (fetch->Connection)
        evhttp_connection_set_closecb(fetch->Connection, nullptr, nullptr);

    nlohmann::json response = nlohmann::json::parse(body, nullptr, false);
    if (ok && httpStatus >= 100 && httpStatus <= 599 && !response.is_discarded()) {
        mCore->Out("MarketplaceProductInfoHandler", "Forwarded product info for assetId={} from upstream (HTTP {})",
            fetch->AssetId, httpStatus);
        SendJson(fetch->Request, static_cast<int>(httpStatus), response);
        return;
    }

    mCore->Out("MarketplaceProductInfoHandler", "Product-info fallback failed for assetId={}: ok={} http={}",
        fetch->AssetId, ok, httpStatus);
    SendMissing(fetch->Request);
}

void MarketplaceProductInfoHandler::BeginRobloxFetch(evhttp_request *req, int64_t assetId) {
    Core *core = mServerEmulator->GetCore();
    Registry *registry = core->GetRegistry();
    const bool proxyEnabled = registry != nullptr
        && registry->GetKeyValue<bool>("emu.enable_roblox_proxy").value_or(true);
    if (!proxyEnabled || !mProxyActive) {
        SendMissing(req);
        return;
    }

    const std::string urlTemplate = registry->GetKeyValue<std::string>("internet.roblox.asset_details")
        .value_or("https://economy.roblox.com/v2/assets/{}/details");

    auto fetch = std::make_shared<ProxyFetch>();
    fetch->Request = req;
    fetch->Connection = evhttp_request_get_connection(req);
    fetch->AssetId = assetId;
    try {
        fetch->Url = std::vformat(urlTemplate, std::make_format_args(assetId));
    } catch (const std::format_error &error) {
        mCore->Out("MarketplaceProductInfoHandler", "Invalid internet.roblox.asset_details template: {}", error.what());
        SendMissing(req);
        return;
    }

    // A configured compatible proxy should never receive the user's Roblox credential. Authentication
    // is forwarded only when the resolved upstream is an actual roblox.com host.
    std::string upstreamHost = Url(fetch->Url).GetHostName();
    for (char &character : upstreamHost)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    const bool isRobloxHost = upstreamHost == "roblox.com" || upstreamHost.ends_with(".roblox.com");
    if (isRobloxHost) {
        if (auto *account = core->GetRbxKeychain()->GetActiveAccount())
            fetch->Cookie = ".ROBLOSECURITY=" + account->Token + ";";
    }

    if (fetch->Connection)
        evhttp_connection_set_closecb(fetch->Connection,
            &MarketplaceProductInfoHandler::OnClientDisconnect, fetch.get());
    mActiveFetches.insert(fetch);

    {
        std::lock_guard<std::mutex> lock(mProxyMutex);
        mProxyQueue.push_back(fetch);
    }
    mProxyCv.notify_one();
}

void MarketplaceProductInfoHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::string assetIdString = GetQueryParam(evhttp_request_get_uri(req), "assetId");
    if (assetIdString.empty())
        assetIdString = mServerEmulator->GetRouteParam("assetId");

    char *end = nullptr;
    const int64_t assetId = strtoll(assetIdString.c_str(), &end, 10);
    if (assetIdString.empty() || end == nullptr || *end != '\0' || assetId <= 0) {
        SendJson(req, 400, {{"errors", nlohmann::json::array({
            {{"code", 0}, {"message", "Invalid asset id"}}
        })}});
        return;
    }

    const std::optional<EmuDb::AssetProductInfo> info = mEmuDbManager->GetAssetProductInfo(assetId);
    if (!info.has_value()) {
        mCore->Out("MarketplaceProductInfoHandler", "No local product info for assetId={}; trying proxy fallback", assetId);
        auto robloxFallback = [this, assetId](evhttp_request *request) {
            BeginRobloxFetch(request, assetId);
        };
        if (!mServerEmulator->TryProxyRequest(req, robloxFallback))
            robloxFallback(req);
        return;
    }

    const EmuDb::AssetSummary &asset = info->Summary;
    const bool groupOwned = asset.GroupId.has_value() && asset.GroupId.value() != 0;
    const int64_t creatorId = groupOwned ? asset.GroupId.value() : asset.UserId.value_or(1);
    const ItemType creatorType = groupOwned ? ItemType::Group : ItemType::User;
    const std::string creatorName = mEmuDbManager->GetItemName(creatorType, creatorId)
        .value_or(groupOwned ? "Group" : "Player");

    nlohmann::json creator = {
        {"Id", creatorId},
        {"Name", creatorName},
        {"CreatorType", groupOwned ? "Group" : "User"},
        {"CreatorTargetId", creatorId},
        {"HasVerifiedBadge", false},
    };

    nlohmann::json priceInRobux = nullptr;
    nlohmann::json priceInTickets = nullptr;
    if (info->CurrencyType.has_value() && info->Price.has_value()) {
        if (info->CurrencyType.value() == static_cast<int>(Roblox::CurrencyType::Robux))
            priceInRobux = info->Price.value();
        else if (info->CurrencyType.value() == static_cast<int>(Roblox::CurrencyType::Tix))
            priceInTickets = info->Price.value();
    }

    const bool isForSale = info->CurrencyType.has_value() && info->Price.has_value();
    nlohmann::json product = {
        {"TargetId", asset.Id},
        {"ProductType", "User Product"},
        {"AssetId", asset.Id},
        {"ProductId", 0},
        {"Name", asset.Name},
        {"Description", asset.Description},
        {"AssetTypeId", asset.Type},
        {"Creator", std::move(creator)},
        {"IconImageAssetId", info->ImageId},
        {"Created", UnixToIso(asset.Created)},
        {"Updated", UnixToIso(asset.Updated)},
        {"PriceInRobux", std::move(priceInRobux)},
        {"PriceInTickets", std::move(priceInTickets)},
        {"Sales", info->Sales},
        {"IsNew", info->IsNew},
        {"IsForSale", isForSale},
        {"IsPublicDomain", info->Public},
        {"IsLimited", info->LimitedType == static_cast<int>(Roblox::LimitedType::Limited)},
        {"IsLimitedUnique", info->LimitedType == static_cast<int>(Roblox::LimitedType::LimitedUnique)},
        {"Remaining", info->Remaining.has_value() ? nlohmann::json(info->Remaining.value()) : nlohmann::json(nullptr)},
        {"MinimumMembershipLevel", info->MinimumMembershipLevel},
        {"ContentRatingTypeId", info->ContentRatingTypeId},
        {"SaleAvailabilityLocations", nullptr},
        {"SaleLocation", nullptr},
        {"CollectibleItemId", nullptr},
        {"CollectibleProductId", nullptr},
        {"CollectiblesItemDetails", nullptr},
        {"TimedOptions", nullptr},
    };

    SendJson(req, HTTP_OK, product);
}
