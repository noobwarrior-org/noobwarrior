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
// File: AssetHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description: HTTP request handler that simulates the action of getting an asset from Roblox services.
//              Some assistance by Claude Opus 4.8
#include <cpr/cpr.h>

#include <NoobWarrior/HttpServer/Emulator/AssetHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

#include <NoobWarrior/HttpServer/Emulator/VideoHandler.h>
#include <NoobWarrior/HttpServer/Base/HttpRange.h>
#include <NoobWarrior/HttpServer/Emulator/EmulatorProxy.h>


#include "../../algorithm/gzip.h"

#include <curl/curl.h>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

// For some reason my build of Studio retrieves the material textures over the internet for some God knows what reason
// So I have to work around it by doing this
static std::unordered_map<std::string, std::string> materials = {
    {"rbxmtl-aluminum-diffuse.dds", "aluminum/diffuse.dds"},
    {"rbxmtl-aluminum-normaldetail.dds", "aluminum/normaldetail.dds"},
    {"rbxmtl-aluminum-normal.dds", "aluminum/normal.dds"},
    {"rbxmtl-aluminum-reflection.dds", "aluminum/reflection.dds"},

    {"rbxmtl-brick-diffuse.dds", "brick/diffuse.dds"},
    {"rbxmtl-brick-normaldetail.dds", "brick/normaldetail.dds"},
    {"rbxmtl-brick-normal.dds", "brick/normal.dds"},
    {"rbxmtl-brick-reflection.dds", "brick/reflection.dds"},

    {"rbxmtl-cobblestone-diffuse.dds", "cobblestone/diffuse.dds"},
    {"rbxmtl-cobblestone-normaldetail.dds", "cobblestone/normaldetail.dds"},
    {"rbxmtl-cobblestone-normal.dds", "cobblestone/normal.dds"},
    {"rbxmtl-cobblestone-reflection.dds", "cobblestone/reflection.dds"},

    {"rbxmtl-concrete-diffuse.dds", "concrete/diffuse.dds"},
    {"rbxmtl-concrete-normaldetail.dds", "concrete/normaldetail.dds"},
    {"rbxmtl-concrete-normal.dds", "concrete/normal.dds"},
    {"rbxmtl-concrete-reflection.dds", "concrete/reflection.dds"},

    {"rbxmtl-diamondplate-diffuse.dds", "diamondplate/diffuse.dds"},
    {"rbxmtl-diamondplate-normaldetail.dds", "diamondplate/normaldetail.dds"},
    {"rbxmtl-diamondplate-normal.dds", "diamondplate/normal.dds"},
    {"rbxmtl-diamondplate-reflection.dds", "diamondplate/reflection.dds"},

    {"rbxmtl-fabric-diffuse.dds", "fabric/diffuse.dds"},
    {"rbxmtl-fabric-normaldetail.dds", "fabric/normaldetail.dds"},
    {"rbxmtl-fabric-normal.dds", "fabric/normal.dds"},
    {"rbxmtl-fabric-reflection.dds", "fabric/reflection.dds"},

    {"rbxmtl-glass-diffuse.dds", "glass/diffuse.dds"},
    {"rbxmtl-glass-normaldetail.dds", "glass/normaldetail.dds"},
    {"rbxmtl-glass-normal.dds", "glass/normal.dds"},
    {"rbxmtl-glass-reflection.dds", "glass/reflection.dds"},

    {"rbxmtl-granite-diffuse.dds", "granite/diffuse.dds"},
    {"rbxmtl-granite-normaldetail.dds", "granite/normaldetail.dds"},
    {"rbxmtl-granite-normal.dds", "granite/normal.dds"},
    {"rbxmtl-granite-reflection.dds", "granite/reflection.dds"},

    {"rbxmtl-grass-diffuse.dds", "grass/diffuse.dds"},
    {"rbxmtl-grass-normaldetail.dds", "grass/normaldetail.dds"},
    {"rbxmtl-grass-normal.dds", "grass/normal.dds"},
    {"rbxmtl-grass-reflection.dds", "grass/reflection.dds"},

    {"rbxmtl-ice-diffuse.dds", "ice/diffuse.dds"},
    {"rbxmtl-ice-normaldetail.dds", "ice/normaldetail.dds"},
    {"rbxmtl-ice-normal.dds", "ice/normal.dds"},
    {"rbxmtl-ice-reflection.dds", "ice/reflection.dds"},

    {"rbxmtl-marble-diffuse.dds", "marble/diffuse.dds"},
    {"rbxmtl-marble-normaldetail.dds", "marble/normaldetail.dds"},
    {"rbxmtl-marble-normal.dds", "marble/normal.dds"},
    {"rbxmtl-marble-reflection.dds", "marble/reflection.dds"},

    {"rbxmtl-metal-diffuse.dds", "metal/diffuse.dds"},
    {"rbxmtl-metal-normaldetail.dds", "metal/normaldetail.dds"},
    {"rbxmtl-metal-normal.dds", "metal/normal.dds"},
    {"rbxmtl-metal-reflection.dds", "metal/reflection.dds"},

    {"rbxmtl-pebble-diffuse.dds", "pebble/diffuse.dds"},
    {"rbxmtl-pebble-normaldetail.dds", "pebble/normaldetail.dds"},
    {"rbxmtl-pebble-normal.dds", "pebble/normal.dds"},
    {"rbxmtl-pebble-reflection.dds", "pebble/reflection.dds"},

    {"rbxmtl-plastic-diffuse.dds", "plastic/diffuse.dds"},
    {"rbxmtl-plastic-normaldetail.dds", "plastic/normaldetail.dds"},
    {"rbxmtl-plastic-normal.dds", "plastic/normal.dds"},

    {"rbxmtl-rust-diffuse.dds", "rust/diffuse.dds"},
    {"rbxmtl-rust-normaldetail.dds", "rust/normaldetail.dds"},
    {"rbxmtl-rust-normal.dds", "rust/normal.dds"},
    {"rbxmtl-rust-reflection.dds", "rust/reflection.dds"},

    {"rbxmtl-sand-diffuse.dds", "sand/diffuse.dds"},
    {"rbxmtl-sand-normaldetail.dds", "sand/normaldetail.dds"},
    {"rbxmtl-sand-normal.dds", "sand/normal.dds"},
    {"rbxmtl-sand-reflection.dds", "sand/reflection.dds"},

    {"rbxmtl-slate-diffuse.dds", "slate/diffuse.dds"},
    {"rbxmtl-slate-normaldetail.dds", "slate/normaldetail.dds"},
    {"rbxmtl-slate-normal.dds", "slate/normal.dds"},
    {"rbxmtl-slate-reflection.dds", "slate/reflection.dds"},

    {"rbxmtl-wood-diffuse.dds", "wood/diffuse.dds"},
    {"rbxmtl-wood-normaldetail.dds", "wood/normaldetail.dds"},
    {"rbxmtl-wood-normal.dds", "wood/normal.dds"},
    {"rbxmtl-wood-reflection.dds", "wood/reflection.dds"},

    {"rbxmtl-woodplanks-diffuse.dds", "woodplanks/diffuse.dds"},
    {"rbxmtl-woodplanks-normaldetail.dds", "woodplanks/normaldetail.dds"},
    {"rbxmtl-woodplanks-normal.dds", "woodplanks/normal.dds"},
    {"rbxmtl-woodplanks-reflection.dds", "woodplanks/reflection.dds"}
};

using namespace NoobWarrior;

AssetHandler::AssetHandler(ServerEmulator *srv, EmuDbManager *dbm) :
    mServerEmulator(srv),
    mEmuDbManager(dbm)
{
    StartProxyPool();
}

AssetHandler::~AssetHandler() {
    StopProxyPool();
}

void AssetHandler::StartProxyPool() {
    bool expected = false;
    if (!mPoolRunning.compare_exchange_strong(expected, true))
        return;
    for (size_t i = 0; i < kProxyThreadCount; i++)
        mProxyThreads.emplace_back(&AssetHandler::RunProxyWorker, this);
}

void AssetHandler::StopProxyPool() {
    if (!mPoolRunning.exchange(false))
        return;
    mProxyCv.notify_all();
    for (auto &t : mProxyThreads)
        if (t.joinable())
            t.join();
    mProxyThreads.clear();
}

void AssetHandler::PauseProxy() {
    // The server is about to free its evhttp, so any fetch still running must not try to reply.
    // Mark replies off and detach every in-flight request from its (soon-to-be-freed) connection.
    // The worker threads keep their own shared_ptr to each ProxyFetch, so this stays safe.
    mProxyActive = false;
    for (auto &fetch : mActiveFetches) {
        if (fetch->Connection)
            evhttp_connection_set_closecb(fetch->Connection, nullptr, nullptr);
        fetch->ClientConnected = false;
    }
    mActiveFetches.clear();
}

void AssetHandler::ResumeProxy() {
    mProxyActive = true;
}

void AssetHandler::OnClientDisconnect(evhttp_connection *conn, void *arg) {
    // The client hung up before its fetch finished. Just note it; OnFetchComplete will see the flag
    // and skip the reply. (arg stays valid: mActiveFetches holds the ProxyFetch until then.)
    static_cast<ProxyFetch*>(arg)->ClientConnected = false;
}

void AssetHandler::RunProxyWorker() {
    // One session per worker, reused across fetches. Because every fetch hits the same host
    // (assetdelivery.roblox.com), curl keeps the TLS connection alive instead of doing a fresh
    // handshake for each asset, which is the bulk of the per-asset cost.
    cpr::Session session;
    session.SetUserAgent(cpr::UserAgent{"Roblox/WinINet"});
    session.SetTimeout(cpr::Timeout{20000});             // 20s per attempt
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

        // A federated joiner's home master is tried first: it holds the item they are actually
        // wearing, and hitting Roblox for it would either miss or return the wrong thing.
        if (fetch->TryFederated) {
            std::vector<unsigned char> fedData;
            std::string fedHash;
            if (mServerEmulator->TryFetchFederatedAsset(fetch->Id, &fedData, &fedHash)) {
                mServerEmulator->GetCore()->RunOnEventLoop(
                    [this, fetch, body = std::move(fedData)]() mutable {
                        OnFetchComplete(fetch, true, 200, std::move(body));
                    });
                continue;
            }
            // Nothing federated had it. Fall through to Roblox, or give up if that's disabled.
            if (!fetch->TryRoblox) {
                mServerEmulator->GetCore()->RunOnEventLoop([this, fetch]() {
                    OnFetchComplete(fetch, false, 0, std::vector<unsigned char>{});
                });
                continue;
            }
        }

        session.SetUrl(cpr::Url{fetch->Url});
        // SetHeader replaces every header, so rebuild it fresh each time. The Accept header is what
        // makes assetdelivery hand back the right *representation* of a material/terrain texture pack
        // (e.g. "rbx-format/norm_dxt" -> the normal DXT). Without it Roblox returns the baseline XML
        // descriptor, which Studio can't decode as a texture, so materials render black.
        cpr::Header reqHeaders;
        if (!fetch->Cookie.empty())
            reqHeaders["Cookie"] = fetch->Cookie;
        if (!fetch->Accept.empty())
            reqHeaders["Accept"] = fetch->Accept;
        session.SetHeader(reqHeaders);

        // Retry transient failures (dropped connections, rate limits, 5xx) before giving up, so a
        // momentary hiccup under load doesn't turn into a missing asset in Studio.
        bool ok = false;
        long httpStatus = 0;
        std::vector<unsigned char> body;
        for (int attempt = 0; attempt < kProxyMaxAttempts && mPoolRunning; attempt++) {
            cpr::Response response = session.Get();
            ok = (response.error.code == cpr::ErrorCode::OK);
            httpStatus = response.status_code;

            bool transient = !ok || httpStatus == 429 || httpStatus == 500
                          || httpStatus == 502 || httpStatus == 503 || httpStatus == 504;
            if (!transient || attempt == kProxyMaxAttempts - 1) {
                body.assign(response.text.begin(), response.text.end());
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150 * (attempt + 1)));
        }

        // The fetch is done; everything else (the reply, the DB write) happens on the event loop.
        mServerEmulator->GetCore()->RunOnEventLoop(
            [this, fetch, ok, httpStatus, body = std::move(body)]() mutable {
                OnFetchComplete(fetch, ok, httpStatus, std::move(body));
            });
    }
}

void AssetHandler::OnFetchComplete(std::shared_ptr<ProxyFetch> fetch, bool ok, long httpStatus, std::vector<unsigned char> data) {
    mActiveFetches.erase(fetch); // this request is finished, one way or another

    if (!mProxyActive)            return; // server stopped; don't touch evhttp
    if (!fetch->ClientConnected)  return; // client hung up; its request is already gone

    // Done with the disconnect notification now that we're about to reply.
    if (fetch->Connection)
        evhttp_connection_set_closecb(fetch->Connection, nullptr, nullptr);

    SqlDb::Response res = fetch->MissResult;
    if (ok && httpStatus == 200 && !data.empty()) {
        // A federated hit is already logged by TryFetchFederatedAsset, and didn't come from Roblox.
        if (fetch->TryRoblox)
            Out("AssetHandler", "Forwarded asset id={} ver={} from Roblox ({} bytes)", fetch->Id, fetch->Version, data.size());
        res = SqlDb::Response::Success;
        if (fetch->SaveToGrabDb)
            SaveGrabbedAsset(fetch->GrabDbPath, fetch->Id, fetch->Version, data);
    } else {
        Out("AssetHandler", "Asset fallback failed for id={} ver={}: ok={} http={}", fetch->Id, fetch->Version,
            ok, httpStatus);
        data.clear();
    }

    ReplyWithAsset(fetch->Request, res, data, "");
}

void AssetHandler::SaveGrabbedAsset(const std::string &dbFilePath, int64_t id, int version,
                                    const std::vector<unsigned char> &data) {
    EmuDb* db = mServerEmulator->GetCore()->GetEmuDbManager()->GetDbFromFilePath(std::filesystem::path(dbFilePath));
    if (db == nullptr)
        return;

    Out("AssetHandler", "Saving asset id={} to database \"{}\"", id, dbFilePath);

    // Save the asset + its data blob immediately with a placeholder name. The real
    // name/description/type/creator and a thumbnail are filled in asynchronously by the
    // AssetEnricher (Roblox's per-asset details endpoint is rate-limited to ~1/min).
    if (!db->DoesItemExist(ItemType::Asset, id)) {
        SqlRow row;
        row.push_back({"Id", (int64_t)id});
        row.push_back({"Name", std::to_string(id)});
        row.push_back({"Type", 0});

        SqlDb::Response addRes = db->AddItem(ItemType::Asset, row);
        if (addRes != SqlDb::Response::Success)
            Out("AssetHandler", "Failed to add asset id={} to database (code={})", id, (int)addRes);
    }

    SqlDb::Response attachRes = db->AttachDataToAsset(id, version, data);
    if (attachRes != SqlDb::Response::Success)
        Out("AssetHandler", "Failed to attach data to asset id={} (code={})", id, (int)attachRes);
    else
        db->MarkDirty();

    mServerEmulator->GetAssetEnricher()->Enqueue(dbFilePath, id);
}

void AssetHandler::AddPlaceIdHeader(evhttp_request *req) {
    std::optional<int64_t> placeId = mServerEmulator->GetCurrentPlaceId();
    if (!placeId.has_value())
        return;

    std::string value = std::to_string(placeId.value());
    evhttp_add_header(evhttp_request_get_output_headers(req), "Roblox-Place-Id", value.c_str());
}

void AssetHandler::ReplyWithAsset(evhttp_request *req, SqlDb::Response res,
                                  const std::vector<unsigned char> &data, const std::string &hash) {
    std::string contentDispositionVal;
    evbuffer* buf = evbuffer_new();
    switch (res) {
    case SqlDb::Response::Success: {
        if (data.empty()) {
            evhttp_send_error(req, 500, "Asset data is empty");
            break;
        }

        // Roblox serves some asset bodies (notably SolidModel/CSG, and some meshes/physics)
        // gzip-compressed. The player's content providers read the raw HTTP body and do NOT honour
        // Content-Encoding, so forwarding the gzip stream verbatim makes them reject the asset
        // ("Unrecognized format: \x1f\x8b\x08"), which leaves Unions/MeshParts with no geometry and
        // the engine then crashes building the CSG cluster from null data. Inflate server-side and
        // send identity bytes. (Non-gzip assets -- textures, images -- pass through untouched; if an
        // inflate ever fails we fall back to the original bytes.)
        const std::vector<unsigned char>* out = &data;
        std::vector<unsigned char> inflated;
        if (IsGzip(data.data(), data.size())) {
            inflated = GzipInflate(data.data(), data.size());
            if (!inflated.empty())
                out = &inflated;
        }

        contentDispositionVal = std::format("attachment; filename=\"{}\"", !hash.empty() ? hash : "asset");

        evkeyvalq *outHeaders = evhttp_request_get_output_headers(req);
        evhttp_add_header(outHeaders, "Content-Type", "application/octet-stream");
        evhttp_add_header(outHeaders, "Content-Disposition", contentDispositionVal.c_str());
        AddPlaceIdHeader(req);

        const uint64_t bodySize = static_cast<uint64_t>(out->size());
        evhttp_add_header(outHeaders, "Accept-Ranges", "bytes");

        const char *rangeHeader = evhttp_find_header(evhttp_request_get_input_headers(req), "Range");
        uint64_t start = 0, end = bodySize > 0 ? bodySize - 1 : 0;

        if (rangeHeader != nullptr && ParseHttpRange(rangeHeader, bodySize, &start, &end)) {
            const std::string contentRange = "bytes " + std::to_string(start) + "-"
                                           + std::to_string(end) + "/" + std::to_string(bodySize);
            evhttp_add_header(outHeaders, "Content-Range", contentRange.c_str());
            evbuffer_add(buf, out->data() + start, static_cast<size_t>(end - start + 1));
            Out("AssetHandler", "  served {} of {} bytes for range {}", end - start + 1, bodySize,
                rangeHeader);
            evhttp_send_reply(req, 206, "Partial Content", buf);
            break;
        }

        if (rangeHeader != nullptr && IsUnsatisfiableSingleRange(rangeHeader)) {
            evhttp_send_error(req, 416, "Requested Range Not Satisfiable");
            break;
        }

        evbuffer_add(buf, out->data(), out->size());
        evhttp_send_reply(req, 200, NULL, buf);
        break;
    }
    case SqlDb::Response::NotFound:
        evhttp_send_error(req, 404, "Asset not found");
        break;
    case SqlDb::Response::MissingBlob:
        evhttp_send_error(req, 500, "Asset blob is missing");
        break;
    default:
        evhttp_send_error(req, 500, "Failed to retrieve asset data");
        break;
    }
    evbuffer_free(buf);
}

static constexpr const char* kResolvedParam = "dontRedirect";

bool AssetHandler::RedirectToSelfForResolution(evhttp_request *req, const char *uri, evkeyvalq *query) {
    if (evhttp_find_header(query, kResolvedParam) != nullptr)
        return false; // second hit: serve the bytes
    if (evhttp_find_header(query, "expectedAssetType") == nullptr &&
        evhttp_find_header(query, "serverplaceid") == nullptr)
        return false;
    
    std::string location = "https://assetdelivery.roblox.com" + std::string(uri ? uri : "");
    location += (location.find('?') == std::string::npos) ? '?' : '&';
    location += kResolvedParam;
    location += "=true";

    Out("AssetHandler", "  resolving via redirect -> {}", location);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", location.c_str());
    evhttp_send_reply(req, 302, "Found", nullptr);
    return true;
}

void AssetHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("AssetHandler", "{}:{} requested asset {}", peer_address, peer_port, uri);

    evkeyvalq headers;
    if (evhttp_parse_query(uri, &headers) != 0) {
        evhttp_send_error(req, 500, "Failed to parse URL parameters");
        return;
    }
    ScopedHeaders headersGuard(&headers);

    const char* idStr = evhttp_find_header(&headers, "id");
    if (idStr == NULL) {
        evhttp_send_error(req, 400, "Id parameter not given");
        return;
    }

    if (RedirectToSelfForResolution(req, uri, &headers))
        return;


    // 0.574 negotiates material/terrain textures via the Accept header (rbx-format/{color,norm,spec}_dxt
    // and ktx/dxt): the SAME asset id returns a DIFFERENT texture per representation. Our local map and
    // the EmuDb cache are keyed by id only, so they'd hand back one blob (often a stale baseline grabbed
    // without this header) for all of colour/normal/specular -> flat, untextured materials. For these
    // requests skip the map and cache and always proxy, forwarding the Accept so each representation is
    // fetched correctly. (Guarded to numeric ids so legacy rbxmtl-*.dds filename lookups are untouched.)
    const char *acceptHdr = evhttp_find_header(evhttp_request_get_input_headers(req), "Accept");
    bool isMaterialFormat = acceptHdr &&
        (std::string_view(acceptHdr).starts_with("rbx-format/") ||
         std::string_view(acceptHdr) == "ktx/dxt");
    /* The batch reply put the engine's chosen representation into this URL (see AssetBatchHandler).
     * Carry it upstream: assetdelivery answers a bare id with a 268-byte
     * "<roblox><texturepack_version>" descriptor rather than a texture, so a fetch that loses the
     * specifier returns something the engine cannot decode. These must also never be answered from the
     * blob cache, which is keyed by id alone and cannot tell one representation from another. */
    std::string upstreamQuery;
    bool hasRepresentation = false;
    bool wantsHlsVideo = false;
    for (const char *param : {"contentRepresentationPriorityList",
                              "doNotFallbackToBaselineRepresentation",
                              "assetResolutionMode"}) {
        const char *value = evhttp_find_header(&headers, param);
        if (value == nullptr || *value == 0)
            continue;
        char *encoded = evhttp_uriencode(value, -1, 0);
        if (encoded == nullptr)
            continue;
        upstreamQuery += std::string("&") + param + "=" + encoded;
        free(encoded);

        /* A video asks for the "hls" representation, and unlike a material texture that request can be
         * answered locally -- one video id has one body, not one per representation. Letting it set
         * hasRepresentation would force needFetch below and send every locally stored video straight
         * past the database to Roblox, so the bypass stays material-only. The parameter is still
         * forwarded upstream, because the Roblox fallback does need it. */
        if (std::string_view(param) == "contentRepresentationPriorityList" && VideoHandler::AcrRequestsHls(value)) {
            wantsHlsVideo = true;
            continue;
        }

        if (std::string_view(param) != "doNotFallbackToBaselineRepresentation")
            hasRepresentation = true;
    }

    bool numericId = idStr[0] != '\0';
    for (const char *p = idStr; *p; ++p)
        if (!std::isdigit(static_cast<unsigned char>(*p))) { numericId = false; break; }
    bool bypassForMaterial = (isMaterialFormat || hasRepresentation) && numericId;

    if (!bypassForMaterial && !mServerEmulator->GetRunningInstances().empty()) {
        auto idCppStr = std::string(idStr);
        const std::string* relPath = nullptr;
        if (auto it = materials.find(idCppStr); it != materials.end())
            relPath = &it->second;
        if (relPath != nullptr) {
            std::filesystem::path engineDir = mServerEmulator->GetCore()->GetEngineDirectory({
                .Side = mServerEmulator->GetRunningInstances().at(0).Side,
                .Version = mServerEmulator->GetRunningInstances().at(0).Version
            });
            std::filesystem::path fileDir = engineDir / "PlatformContent" / "pc" / "textures" / *relPath;

            if (!std::filesystem::exists(fileDir)) {
                evhttp_send_error(req, 500, "Material file doesn't exist");
                return;
            }

            std::ifstream stream(fileDir, std::ios::binary);
            if (stream.fail()) {
                evhttp_send_error(req, 500, "Failed to open material file");
                return;
            }

            auto fileSize = std::filesystem::file_size(fileDir);
            std::vector<unsigned char> data(fileSize);
            if (!stream.read(reinterpret_cast<char*>(data.data()), fileSize)) {
                evhttp_send_error(req, 500, "Failed to read material file");
                return;
            }

            evbuffer* buf = evbuffer_new();

            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Disposition", "attachment; filename=\"material.dds\"");
            AddPlaceIdHeader(req);

            evbuffer_add(buf, data.data(), data.size());
            evhttp_send_reply(req, 200, NULL, buf);

            evbuffer_free(buf);
            return;
        }
    }

    const char* verStr = evhttp_find_header(&headers, "version");
    int ver = 0;
    if (verStr != NULL) {
        char *verEndPtr;
        ver = strtol(verStr, &verEndPtr, 10);
        if (verStr == verEndPtr) {
            /* strtol failed */
            evhttp_send_error(req, 400, "Invalid version");
            return;
        }
    }

    char *idEndPtr;
    int64_t id = strtoll(idStr, &idEndPtr, 10);
    if (idStr == idEndPtr) {
        /* strtoll failed */
        evhttp_send_error(req, 400, "Invalid ID");
        return;
    }

    /* A video the engine wants as HLS gets sent to VideoHandler instead of answered here.
     *
     * The redirect is not cosmetic: HlsInputFormat resolves every relative URI in a playlist against
     * the URL the playlist arrived from, so a playlist served from /v1/asset/?id=N would make "seg0.ts"
     * resolve to /v1/asset/seg0.ts and lose the id. Redirecting first means the segment names resolve
     * against /video/v1/{id}/{hash}/ and come back addressable. RedirectToSelfForResolution above
     * already relies on the engine following a 302 on this endpoint.
     *
     * ResolvePlaylistPath only names a playlist whose segments already exist, so this never redirects
     * the engine at work that has not been done; a video that is not ready yet falls through and is
     * served whole below, which the engine can still demux. If the asset is not here at all we also
     * fall through and the proxy/Roblox chain answers it -- a remote host issues its own redirect,
     * which reaches the client through the proxy unchanged. */
    if (wantsHlsVideo) {
        const std::string location = VideoHandler::ResolvePlaylistPath(mServerEmulator, id, ver);
        if (!location.empty()) {
            Out("AssetHandler", "  video asset {} -> {}", id, location);
            evhttp_add_header(evhttp_request_get_output_headers(req), "Location", location.c_str());
            // The redirect target embeds the blob hash, so it changes whenever the video does -- and a
            // cached redirect would send the engine to segments that no longer exist. The playlist it
            // lands on is served no-store for the same reason.
            evhttp_add_header(evhttp_request_get_output_headers(req), "Cache-Control",
                              "no-store, no-cache, must-revalidate");
            evhttp_send_reply(req, 302, "Found", nullptr);
            return;
        }
    }

    std::vector<unsigned char> data;
    std::string hash;
    SqlDb::Response res = mEmuDbManager->RetrieveAssetData(id, ver, &data, &hash);

    bool needFetch = bypassForMaterial
                  || res == SqlDb::Response::NotFound
                  || res == SqlDb::Response::MissingBlob
                  || (res == SqlDb::Response::Success && data.empty());
    
    if (!needFetch) {
        ReplyWithAsset(req, res, data, hash);
        return;
    }

    // A joined federated player's worn items (and the meshes/textures the engine fetches for them) live
    // on their home master. That lookup is an HTTP call, so it happens on a proxy worker alongside the
    // Roblox fallback rather than here — blocking the event loop would deadlock against a master server
    // hosted in this same program. Skipped for material bypass (those representations legitimately come
    // from Roblox).
    bool tryFederated = !bypassForMaterial && mServerEmulator->HasFederatedOrigins();

    bool proxyEnabled = mServerEmulator->GetCore()->GetRegistry()->GetKeyValue<bool>("emu.enable_roblox_proxy").value_or(true);
    auto robloxFallback = [this, id, ver, res, proxyEnabled, tryFederated, upstreamQuery](evhttp_request *r) {
        // Either source needs the worker; only give up outright when neither is available.
        if (proxyEnabled || tryFederated)
            BeginProxyFetch(r, id, ver, res, upstreamQuery, tryFederated, proxyEnabled); // reply happens later, in OnFetchComplete
        else
            ReplyWithAsset(r, res, std::vector<unsigned char>{}, std::string{});
    };

    /* For a video, log what the host's emulator actually relayed back.
     *
     * A joiner cannot resolve a remote video locally -- the blob lives on the host, so the hash lookup
     * above fails and no redirect is issued -- which means the whole thing rides on the host answering
     * this proxied request with a playlist. cpr follows the host's 302 to /video/v1/... and the layer
     * reports 200, so a failure here is otherwise completely silent: no /video/v1/ request ever appears
     * in the joiner's log and there is nothing to distinguish "the host sent a playlist we mishandled"
     * from "the host sent something else entirely". The body is passed through untouched. */
    EmulatorProxy::ResponseTransform videoProbe;
    if (wantsHlsVideo) {
        videoProbe = [id](std::vector<unsigned char> body) {
            const std::string head(reinterpret_cast<const char*>(body.data()),
                                   std::min<size_t>(body.size(), 7));
            Out("AssetHandler", "  proxied video asset {}: {} bytes, {}", id, body.size(),
                head == "#EXTM3U" ? "an HLS playlist" : "NOT a playlist (head=\"" + head + "\")");
            return body;
        };
    }

    if (mServerEmulator->TryProxyRequest(req, robloxFallback, std::move(videoProbe)))
        return;
    robloxFallback(req);
}

void AssetHandler::BeginProxyFetch(evhttp_request *req, int64_t id, int version, SqlDb::Response missResult,
                                   const std::string &upstreamQuery, bool tryFederated, bool tryRoblox) {
    Core *core = mServerEmulator->GetCore();

    auto fetch = std::make_shared<ProxyFetch>();
    fetch->Request      = req;
    fetch->Connection   = evhttp_request_get_connection(req);
    fetch->Id           = id;
    fetch->Version      = version;
    fetch->MissResult   = missResult;
    fetch->TryFederated = tryFederated;
    fetch->TryRoblox    = tryRoblox;

    fetch->Url = "https://assetdelivery.roblox.com/v1/asset/?id=" + std::to_string(id);
    if (version > 0)
        fetch->Url += "&version=" + std::to_string(version);
    fetch->Url += upstreamQuery; // else assetdelivery returns a descriptor instead of the texture

    // Forward the engine's Accept header so assetdelivery returns the requested texture
    // representation (rbx-format/{color,norm,spec}_dxt or ktx/dxt) instead of the baseline XML
    // descriptor; the same id yields a different texture per representation.
    if (const char *accept = evhttp_find_header(evhttp_request_get_input_headers(req), "Accept"))
        fetch->Accept = accept;

    if (auto *acc = core->GetRbxKeychain()->GetActiveAccount())
        fetch->Cookie = ".ROBLOSECURITY=" + acc->Token + ";";

    bool grabMode = core->GetRegistry()->GetKeyValue<bool>("emu.asset_grab_mode").value_or(false);
    fetch->GrabDbPath  = grabMode ? core->GetRegistry()->GetKeyValue<std::string>("emu.asset_grab_db").value_or("") : "";
    fetch->SaveToGrabDb = grabMode && !fetch->GrabDbPath.empty();

    // Get told if the client disconnects before the fetch finishes.
    if (fetch->Connection)
        evhttp_connection_set_closecb(fetch->Connection, &AssetHandler::OnClientDisconnect, fetch.get());

    mActiveFetches.insert(fetch);

    {
        std::lock_guard<std::mutex> lock(mProxyMutex);
        mProxyQueue.push_back(fetch);
    }
    mProxyCv.notify_one();
}
