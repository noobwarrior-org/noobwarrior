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
// File: VoiceChatHandler.cpp
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Voice eligibility endpoints and the local Team Test TURN relay.
// (Disclaimer: Assisted by GPT 5.6 Sol)
#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
// iptypes.h exposes GetAdaptersAddresses and the GAA flags only when Winsock 2
// has already been selected. Keep this before project/libevent headers because
// windows.h otherwise selects the legacy Winsock 1 declarations under MinGW.
#include <winsock2.h>
#include <iphlpapi.h>
#endif

#include <NoobWarrior/HttpServer/Emulator/VoiceChatHandler.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <event2/buffer.h>
#include <event2/http.h>

#include <nlohmann/json.hpp>

#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
#include <juice/juice.h>
#endif

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace NoobWarrior;

namespace {
constexpr const char *kUserSettings = R"({"isVoiceEnabled":true,"isUserOptIn":true,"isUserEligible":true,"isBanned":false,"bannedUntil":null,"canVerifyAgeForVoice":true,"isVerifiedForVoice":true,"denialReason":0,"isOptInDisabled":false,"hasEverOpted":true,"isAvatarVideoEnabled":true,"isAvatarVideoOptIn":true,"isAvatarVideoOptInDisabled":false,"isAvatarVideoEligible":true,"hasEverOptedAvatarVideo":true})";

std::optional<int64_t> ParsePositiveId(std::string_view text) {
    int64_t value = 0;
    const auto [end, error] = std::from_chars(
        text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size() || value <= 0)
        return std::nullopt;
    return value;
}

std::optional<int64_t> TrailingPathId(
    std::string_view path, std::string_view prefix) {
    if (!path.starts_with(prefix))
        return std::nullopt;
    return ParsePositiveId(path.substr(prefix.size()));
}

nlohmann::json UniverseSettingsJson(bool enabled) {
    nlohmann::json reasons = nlohmann::json::array();
    if (!enabled)
        reasons.push_back("Voice chat is disabled for this universe.");
    return {
        {"isUniverseEnabledForVoice", enabled},
        {"isPlaceEnabledForVoice", enabled},
        {"reasons", std::move(reasons)},
        {"isUniverseEnabledForAvatarVideo", true},
        {"isPlaceEnabledForAvatarVideo", true},
    };
}

nlohmann::json VerificationOverlayJson(bool enabled) {
    return {
        {"showAgeVerificationOverlay", false},
        {"showVoiceOptInOverlay", false},
        {"showAvatarVideoOptInOverlay", false},
        {"universePlaceVoiceEnabledSettings", UniverseSettingsJson(enabled)},
        {"voiceSettings", nlohmann::json::parse(kUserSettings)},
    };
}

std::string RequestPath(evhttp_request *req) {
    const char *uri = evhttp_request_get_uri(req);
    std::string path = uri ? uri : "";
    if (const size_t query = path.find('?'); query != std::string::npos)
        path.resize(query);

    std::string normalized;
    normalized.reserve(path.size());
    bool previousSlash = false;
    for (const char ch : path) {
        if (ch == '/') {
            if (previousSlash)
                continue;
            previousSlash = true;
        } else {
            previousSlash = false;
        }
        normalized.push_back(ch);
    }
    return normalized;
}

void SendJson(evhttp_request *req, std::string_view json) {
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Cache-Control", "no-store");
    evbuffer *reply = evbuffer_new();
    evbuffer_add(reply, json.data(), json.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, reply);
    evbuffer_free(reply);
}

#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
std::atomic_bool gVerboseTransportLogging {false};

bool IsHighFrequencyTurnLog(std::string_view message) {
    return message.starts_with("Received ChannelData datagram") ||
        message.starts_with("Received STUN datagram") ||
        message.starts_with("Processing STUN Send indication") ||
        message.starts_with("Answering STUN Binding request") ||
        message.starts_with("Got STUN binding from client") ||
        message.starts_with("Ignoring ECONNRESET returned by recvfrom");
}

bool IsUsableTurnIpv4(const sockaddr_in *address) {
    if (address == nullptr)
        return false;

    const auto *octets = reinterpret_cast<const unsigned char *>(
        &address->sin_addr.S_un.S_addr);
    return octets[0] != 0 && octets[0] != 127 && octets[0] < 224 &&
           !(octets[0] == 169 && octets[1] == 254);
}

std::string FindTurnIpv4Address() {
    ULONG bufferSize = 16 * 1024;
    std::vector<unsigned char> buffer(bufferSize);
    constexpr ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
        GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS;
    ULONG result = GetAdaptersAddresses(
        AF_INET, flags, nullptr,
        reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &bufferSize);
    if (result == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufferSize);
        result = GetAdaptersAddresses(
            AF_INET, flags, nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &bufferSize);
    }
    if (result != NO_ERROR)
        return {};

    auto *adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
    for (const bool requireGateway : {true, false}) {
        for (auto *adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp ||
                adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
                adapter->IfType == IF_TYPE_TUNNEL ||
                (requireGateway && adapter->FirstGatewayAddress == nullptr)) {
                continue;
            }

            for (auto *unicast = adapter->FirstUnicastAddress;
                 unicast != nullptr; unicast = unicast->Next) {
                if (unicast->Address.lpSockaddr == nullptr ||
                    unicast->Address.lpSockaddr->sa_family != AF_INET) {
                    continue;
                }
                const auto *address = reinterpret_cast<const sockaddr_in *>(
                    unicast->Address.lpSockaddr);
                if (!IsUsableTurnIpv4(address))
                    continue;

                const auto *octets = reinterpret_cast<const unsigned char *>(
                    &address->sin_addr.S_un.S_addr);
                char text[16] {};
                std::snprintf(text, sizeof(text), "%u.%u.%u.%u",
                    static_cast<unsigned>(octets[0]),
                    static_cast<unsigned>(octets[1]),
                    static_cast<unsigned>(octets[2]),
                    static_cast<unsigned>(octets[3]));
                return text;
            }
        }
    }
    return {};
}

void LogTurnRelay(juice_log_level_t level, const char *message) {
    if (message == nullptr ||
        (!gVerboseTransportLogging.load(std::memory_order_relaxed) &&
         IsHighFrequencyTurnLog(message))) {
        return;
    }

    Out("VoiceTurnRelay", "libjuice[{}]: {}", static_cast<int>(level), message);
}
#endif
}

struct VoiceChatHandler::TurnRelayState {
#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
    juice_server_t *Server {nullptr};
    std::string BindHost;
    std::string AdvertisedHost;
    uint16_t Port {3478};
    uint16_t RelayPortBegin {49160};
    uint16_t RelayPortEnd {49200};
#endif
};

VoiceChatHandler::VoiceChatHandler(ServerEmulator *emulator) :
    mEmulator(emulator),
    mTurnRelay(std::make_unique<TurnRelayState>()) {}

VoiceChatHandler::~VoiceChatHandler() {
    StopTurnRelay();
}

bool VoiceChatHandler::StartTurnRelay() {
#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
    if (mTurnRelay->Server)
        return true;

    Registry *registry = mEmulator && mEmulator->GetCore()
        ? mEmulator->GetCore()->GetRegistry()
        : nullptr;
    mTurnRelay->BindHost = FindTurnIpv4Address();
    if (mTurnRelay->BindHost.empty()) {
        mCore->Out("VoiceTurnRelay", "No usable non-loopback IPv4 interface was found");
        return false;
    }
    mTurnRelay->AdvertisedHost = mEmulator
        ? mEmulator->ResolveAdvertisedAddress(mTurnRelay->BindHost, true)
        : mTurnRelay->BindHost;

    const auto configuredInt = [registry](const char *key, int fallback,
                                          int minimum, int maximum) {
        const int value = registry
            ? registry->GetKeyValue<int>(key).value_or(fallback)
            : fallback;
        return std::clamp(value, minimum, maximum);
    };
    mTurnRelay->Port = static_cast<uint16_t>(configuredInt(
        "emu.voice_turn.port", 3478, 1, 65535));
    mTurnRelay->RelayPortBegin = static_cast<uint16_t>(configuredInt(
        "emu.voice_turn.relay_port_begin", 49160, 1024, 65535));
    mTurnRelay->RelayPortEnd = static_cast<uint16_t>(configuredInt(
        "emu.voice_turn.relay_port_end", 49200, 1024, 65535));
    if (mTurnRelay->RelayPortBegin > mTurnRelay->RelayPortEnd) {
        mCore->Out("VoiceTurnRelay",
            "Invalid relay port range {}-{}; using 49160-49200",
            mTurnRelay->RelayPortBegin, mTurnRelay->RelayPortEnd);
        mTurnRelay->RelayPortBegin = 49160;
        mTurnRelay->RelayPortEnd = 49200;
    }

    // Preserve the validated LAN fallback for older LocalRCC builds. Do not
    // expose that source-known credential on a publicly advertised relay;
    // current LocalRCC obtains a random, expiring credential per request.
    juice_server_credentials_t credentials {};
    credentials.username = "local-turn";
    credentials.password = "local-pass";
    credentials.allocations_quota = 8;
    const bool isPublicRelay =
        mTurnRelay->AdvertisedHost != mTurnRelay->BindHost;

    juice_server_config_t config {};
    config.credentials = isPublicRelay ? nullptr : &credentials;
    config.credentials_count = isPublicRelay ? 0 : 1;
    config.max_allocations = configuredInt(
        "emu.voice_turn.max_allocations", 64, 8, 1024);
    config.max_peers = 64;
    config.bind_address = mTurnRelay->BindHost.c_str();
    config.external_address = mTurnRelay->AdvertisedHost.c_str();
    config.port = mTurnRelay->Port;
    config.relay_port_range_begin = mTurnRelay->RelayPortBegin;
    config.relay_port_range_end = mTurnRelay->RelayPortEnd;
    config.realm = "noobWarrior local voice";

    juice_set_log_handler(&LogTurnRelay);
    const bool verboseTransportLogging = registry != nullptr &&
        registry->GetKeyValue<bool>("debug.log_voice_transport").value_or(false);
    gVerboseTransportLogging.store(verboseTransportLogging, std::memory_order_relaxed);
    // Keep DEBUG enabled so one-time lifecycle diagnostics remain visible. The log
    // callback filters only known packet-frequency messages unless explicitly enabled.
    juice_set_log_level(JUICE_LOG_LEVEL_DEBUG);
    mTurnRelay->Server = juice_server_create(&config);
    if (!mTurnRelay->Server) {
        mCore->Out("VoiceTurnRelay", "Failed to start TURN relay on {}:{}",
            mTurnRelay->BindHost, mTurnRelay->Port);
        return false;
    }

    mCore->Out("VoiceTurnRelay",
        "Started TURN relay bind={}:{} advertised={}:{} relayPorts={}-{}",
        mTurnRelay->BindHost, juice_server_get_port(mTurnRelay->Server),
        mTurnRelay->AdvertisedHost, mTurnRelay->Port,
        mTurnRelay->RelayPortBegin, mTurnRelay->RelayPortEnd);
    return true;
#else
    return false;
#endif
}

void VoiceChatHandler::StopTurnRelay() {
#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
    if (!mTurnRelay || !mTurnRelay->Server)
        return;

    juice_server_destroy(mTurnRelay->Server);
    mTurnRelay->Server = nullptr;
    mCore->Out("VoiceTurnRelay", "Stopped TURN relay");
#endif
}

void VoiceChatHandler::OnRequest(evhttp_request *req, void *userdata) {
    const std::string path = RequestPath(req);
    mCore->Out("VoiceChatHandler", "Requested {}", path);

    const evhttp_cmd_type method = evhttp_request_get_command(req);
    if (method != EVHTTP_REQ_GET && method != EVHTTP_REQ_POST) {
        evhttp_add_header(evhttp_request_get_output_headers(req), "Allow", "GET, POST");
        evhttp_send_error(req, HTTP_BADMETHOD, nullptr);
        return;
    }

    EmuDbManager *databaseManager = mEmulator && mEmulator->GetCore()
        ? mEmulator->GetCore()->GetEmuDbManager()
        : nullptr;
    const auto universeVoiceEnabled = [databaseManager](
        std::optional<int64_t> universeId) {
        if (!databaseManager || !universeId)
            return true;
        return databaseManager->GetUniverseVoiceChatEnabled(*universeId)
            .value_or(true);
    };

    if (path == "/emu/v1/voice/turn-auth") {
#if defined(NOOBWARRIOR_VOICE_TURN_RELAY)
        const char *peer = nullptr;
        uint16_t peerPort = 0;
        evhttp_connection *connection = evhttp_request_get_connection(req);
        if (connection != nullptr)
            evhttp_connection_get_peer(connection, &peer, &peerPort);
        if (method != EVHTTP_REQ_GET || peer == nullptr ||
            !IsLoopbackOrEmpty(peer)) {
            evhttp_send_error(req, HTTP_FORBIDDEN, nullptr);
            return;
        }
        if (!StartTurnRelay()) {
            evhttp_send_error(req, HTTP_SERVUNAVAIL, nullptr);
            return;
        }

        Registry *registry = mEmulator && mEmulator->GetCore()
            ? mEmulator->GetCore()->GetRegistry()
            : nullptr;
        const int ttlSeconds = std::clamp(
            registry ? registry->GetKeyValue<int>(
                "emu.voice_turn.credential_ttl_seconds").value_or(3600)
                : 3600,
            300, 86400);
        const std::string username = "nw-" + AuthUtil::RandomHex(8);
        const std::string password = AuthUtil::RandomHex(32);
        if (username.size() <= 3 || password.empty()) {
            evhttp_send_error(req, HTTP_INTERNAL, nullptr);
            return;
        }

        juice_server_credentials_t credentials {};
        credentials.username = username.c_str();
        credentials.password = password.c_str();
        credentials.allocations_quota = 4;
        const int credentialResult = juice_server_add_credentials(
            mTurnRelay->Server, &credentials,
            static_cast<unsigned long>(ttlSeconds) * 1000UL);
        if (credentialResult != JUICE_ERR_SUCCESS) {
            mCore->Out("VoiceTurnRelay",
                "Could not add short-lived TURN credentials result={}",
                credentialResult);
            evhttp_send_error(req, HTTP_INTERNAL, nullptr);
            return;
        }

        const std::string publicTurnUri = "turn:" + mTurnRelay->AdvertisedHost +
            ":" + std::to_string(mTurnRelay->Port) + "?transport=udp";
        nlohmann::json turnUris = nlohmann::json::array({publicTurnUri});
        if (mTurnRelay->BindHost != mTurnRelay->AdvertisedHost) {
            // LAN clients can use this candidate when the router does not
            // support NAT hairpinning. Internet clients fail it and use the
            // public candidate above.
            turnUris.push_back("turn:" + mTurnRelay->BindHost + ":" +
                std::to_string(mTurnRelay->Port) + "?transport=udp");
        }
        const nlohmann::json response = {
            {"username", username},
            {"password", password},
            {"ttl", ttlSeconds},
            {"uris", std::move(turnUris)},
        };
        SendJson(req, response.dump());
        mCore->Out("VoiceTurnRelay",
            "Issued short-lived TURN auth advertisedUri={} lanFallback={} ttl={}s",
            publicTurnUri,
            mTurnRelay->BindHost != mTurnRelay->AdvertisedHost,
            ttlSeconds);
#else
        evhttp_send_error(req, HTTP_SERVUNAVAIL, nullptr);
#endif
        return;
    }

    if (path == "/v1/settings") {
        SendJson(req, kUserSettings);
        return;
    }
    if (path == "/v1/settings/user-opt-in") {
        SendJson(req, R"({"isUserOptIn":true})");
        return;
    }
    if (path == "/v1/settings/verify/show-overlay") {
        SendJson(req, R"({"showOverlay":false})");
        return;
    }
    if (path.starts_with("/v1/settings/verify/show-age-verification-overlay/")) {
        const auto universeId = TrailingPathId(
            path, "/v1/settings/verify/show-age-verification-overlay/");
        SendJson(req, VerificationOverlayJson(
            universeVoiceEnabled(universeId)).dump());
        return;
    }
    if (const auto universeId = TrailingPathId(
            path, "/v1/settings/universe/")) {
        const bool enabled = universeVoiceEnabled(universeId);
        mCore->Out("VoiceChatHandler", "Universe voice setting universeId={} enabled={}",
            *universeId, enabled);
        SendJson(req, UniverseSettingsJson(enabled).dump());
        return;
    }
    if (path == "/v2/rccsettings/universe") {
        std::optional<int64_t> placeId;
        if (const char *header = evhttp_find_header(
                evhttp_request_get_input_headers(req), "Roblox-Place-Id")) {
            placeId = ParsePositiveId(header);
        }
        const std::optional<int64_t> universeId = placeId && databaseManager
            ? databaseManager->GetUniverseIdForPlace(*placeId)
            : std::nullopt;
        const bool enabled = universeVoiceEnabled(universeId);
        mCore->Out("VoiceChatHandler",
            "RCC universe voice setting placeId={} universeId={} enabled={}",
            placeId.value_or(0), universeId.value_or(0), enabled);
        SendJson(req, UniverseSettingsJson(enabled).dump());
        return;
    }
    if (path == "/v2/rccsettings/user") {
        // ServerEmulator normally starts the relay with the HTTP listener, but
        // also make the voice settings boundary self-healing. This removes a
        // startup-order ambiguity from Team Test and gives the log an explicit
        // readiness record immediately before the Player requests TURN auth.
        const bool turnRelayReady = StartTurnRelay();
        mCore->Out("VoiceChatHandler", "TURN relay ready for /v2/rccsettings/user: {}",
            turnRelayReady);
        SendJson(req, kUserSettings);
        return;
    }

    // These settings writes only record opt-in/upsell state on Roblox. noobWarrior keeps
    // voice locally enabled, so acknowledge them without introducing persistent account state.
    if (path == "/v1/settings/user-opt-in/avatarvideo" ||
        path == "/v1/settings/record-user-seen-avatar-video-upsell-modal" ||
        path == "/v1/settings/record-user-seen-upsell-modal" ||
        path.starts_with("/v1/settings/universe/avatarvideo/")) {
        SendJson(req, R"({"status":"Success"})");
        return;
    }

    evhttp_send_error(req, HTTP_NOTFOUND, nullptr);
}
