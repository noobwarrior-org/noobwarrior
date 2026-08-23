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
// File: VoiceChat719.cpp
// Description: VoiceChatService/WebRTC bridge for exact Studio 0.719.

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>
#include <winhttp.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <MinHook.h>
#include <nlohmann/json.hpp>
#include <rtc/rtc.h>

#include "LocalRccShared.h"
#include "VoiceChat719.h"

using LocalRccShared::Log;
using LocalRccShared::ScanAny;
using LocalRccShared::GetExactStudio719ImageBase;

namespace types {
struct sdp_compression_result_719 {
    int compression_mode;
    int reserved;
    std::string payload;
};
static_assert(sizeof(sdp_compression_result_719) == 40);
using sdp_compress_fn = sdp_compression_result_719 (*)(
    const std::string& plain_sdp, int compression_mode,
    int sdp_message_kind);
using sdp_decompress_fn = std::string (*)(
    const std::string& compressed_sdp, int compression_mode,
    int sdp_message_kind);
using sdp_wrap_fn = std::string (*)(
    const std::string& compressed_sdp, int compression_mode);
using configure_without_run_fn = __int64 (*)(void* context, uint8_t* settings);
using voice_rcc_context_gate_fn = uint8_t (*)(void* service_provider);
using process_voice_remote_event_fn = __int64 (*)(
    uint8_t* voice_chat_service, void* event_descriptor,
    void* arguments, uint64_t sender_context);
}

static types::configure_without_run_fn gConfigureWithoutRun = nullptr;
static types::configure_without_run_fn gOrigConfigureWithoutRun = nullptr;
static types::voice_rcc_context_gate_fn gVoiceRccContextGate = nullptr;
static types::voice_rcc_context_gate_fn gOrigVoiceRccContextGate = nullptr;
static types::process_voice_remote_event_fn gProcessVoiceRemoteEvent = nullptr;
static types::process_voice_remote_event_fn gOrigProcessVoiceRemoteEvent = nullptr;

// -----------------------------------------------------------------------------
// Hook: ConfigureWithoutRun (Studio 0.719 team-test host)
//
// VoiceChatService::EnableDefaultVoice is the per-place default-publishing
// veto. Server voice startup is separately gated by the universe/place
// eligibility returned by the voice settings service.
// In 0.719, the regular Studio launch path initializes the corresponding
// ConfigureWithoutRun bytes to false and never fills them from the Team Test
// command line. The bridge fetches the host Core's settings for this place and
// writes those values before ConfigureWithoutRun copies them to the hidden RCC
// properties.
//
// Keep this at the settings boundary: it avoids depending on VoiceChatService
// object layout and lets Studio create/start its own RCC control plane normally.
// Installation is additionally guarded by the exact 0.719 image fingerprint.
// -----------------------------------------------------------------------------

namespace {
constexpr size_t kVoiceEnabledForUniverseOnRccOffset = 272;
constexpr size_t kVoiceEnabledForPlaceOnRccOffset = 273;
constexpr uintptr_t kVoiceChatEnabledRccProperties2Rva = 0x0c0d52d0;
constexpr uintptr_t kVoiceControlPlaneInitFlowWithVoiceTypeRva = 0x0c0d5370;
constexpr uintptr_t kConfigureWithoutRunVoiceGateReturnRva = 0x02592077;
constexpr uintptr_t kClientRetryJoinWithConfigDescriptorRva = 0x0c063630;
constexpr uintptr_t kClientVoiceCapabilityDescriptorRva = 0x0c064050;
constexpr uintptr_t kClientVoiceCapabilityWithConfigDescriptorRva = 0x0c0640e0;
constexpr uintptr_t kVoicePlayerMuteStatusChangedDescriptorRva = 0x0c0636c0;
constexpr uintptr_t kVoicePlayerMuteStateChangedServerToClientDescriptorRva =
    0x0c064200;
constexpr uintptr_t kPublishStateChangeDescriptorRva = 0x0c0637e0;
constexpr uintptr_t kJoinedVoiceDescriptorRva = 0x0c063870;
constexpr uintptr_t kReJoinedVoiceDescriptorRva = 0x0c063e10;
constexpr uintptr_t kPublishingHandshakeAckedDescriptorRva = 0x0c063900;
constexpr uintptr_t kPublishingHandshakeCompletedDescriptorRva = 0x0c063990;
constexpr uintptr_t kPublishingHandshakeInitiatedDescriptorRva = 0x0c063a20;
constexpr uintptr_t kUpdateTurnAuthInfoRequestDescriptorRva = 0x0c063cf0;
constexpr uintptr_t kUserTurnAuthDescriptorRva = 0x0c063ea0;
constexpr uintptr_t kRelayCandidatesGatheredDescriptorRva = 0x0c063fc0;
constexpr uintptr_t kSubscriptionHandshakeInitiatedDescriptorRva = 0x0c0645f0;
constexpr uintptr_t kSubscriptionHandshakeAckedDescriptorRva = 0x0c064680;
constexpr uintptr_t kSubscriptionHandshakeCompletedDescriptorRva = 0x0c064950;
constexpr uintptr_t kVoiceSubscriptionInitialBatchEmptyDescriptorRva = 0x0c0649e0;
constexpr uintptr_t kSubscriptionFeedStartedDescriptorRva = 0x0c064a70;
constexpr uintptr_t kMuteStateVariantTypeRva = 0x04416fe0;
constexpr uintptr_t kSdpWrapRva = 0x06091310;
constexpr uintptr_t kSdpCompressRva = 0x060914e0;
constexpr uintptr_t kSdpDecompressRva = 0x06091520;
constexpr uintptr_t kStringVariantTypeRva = 0x06135e00;
constexpr uintptr_t kIntVariantTypeRva = 0x06135ac0;
constexpr uintptr_t kInt64VariantTypeRva = 0x06137320;
constexpr uintptr_t kBoolVariantTypeRva = 0x061373f0;
constexpr uintptr_t kArrayVariantTypeRva = 0x06139920;
constexpr uintptr_t kStringVariantStorageOpsRva = 0x00964560;
constexpr uintptr_t kInt64VariantCopyRva = 0x00994e40;
constexpr uintptr_t kVariantNoopDestructorRva = 0x006b8af0;
constexpr uintptr_t kAssignArrayVariantStorageRva = 0x0096e2f0;
constexpr ptrdiff_t kVoiceRccContextGateTailOffset = 0x110;
// Exact 0.719 ServerReplicator/Player layout used to associate a voice feed
// with the Player instance already replicated to each client. The incoming
// RemoteEventSource context is ServerReplicator + 0x128.
constexpr ptrdiff_t kVoiceSenderContextOffset = 0x128;
constexpr ptrdiff_t kServerReplicatorRemotePlayerOffset = 0x4d00;
constexpr ptrdiff_t kPlayerUserIdOffset = 0x2d8;
constexpr ptrdiff_t kPlayerEncodedUserIdOffset = 0x408;
constexpr uint64_t kPlayerUserIdEncodingBias = 0x4dcebd99ULL;
std::atomic_bool gVoiceRccConfiguredEnabled {true};

// Reflection::Variant is 80 bytes in exact 0.719. The payload begins at +16;
// its reflected type and storage-operation table occupy the first two pointers.
// JoinedVoice is (string channelName, int64 participantId, string sessionId,
// string channelId). Keep these POD layouts local to the fingerprinted 0.719
// path rather than exposing an apparent cross-version engine ABI.
struct VoiceVariant719 {
    void* type;
    void* storageOps;
    alignas(8) uint8_t payload[64];
};
static_assert(sizeof(VoiceVariant719) == 80);

struct VoiceVariantVector719 {
    VoiceVariant719* begin;
    VoiceVariant719* end;
    VoiceVariant719* capacity;
};

struct VoiceEventTarget719 {
    uint64_t senderContext;
    uint32_t kind;
    uint32_t reserved;
};

using reflected_type_fn = void* (*)();
using reflected_storage_ops_fn = void* (*)();
using assign_array_variant_storage_fn = void* (*)(
    void* storageAndPayload, const void* sharedValueArray);
using send_remote_event_fn = void (*)(
    uint8_t* service, void* descriptor, VoiceVariantVector719* arguments,
    VoiceEventTarget719* target);
}

#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
struct UniverseVoiceSettings719 {
    bool universeEnabled = true;
    bool placeEnabled = true;
};
static bool FetchUniverseVoiceSettingsFromCore719(
    UniverseVoiceSettings719& settings);
#endif

static bool ReadVoiceStringVariant719(
    const VoiceVariant719& variant, const char** dataOut, size_t* sizeOut) {
    const size_t size = *reinterpret_cast<const size_t*>(variant.payload + 16);
    const size_t capacity = *reinterpret_cast<const size_t*>(variant.payload + 24);
    if (size > capacity || size > 1024 * 1024)
        return false;

    const char* data = capacity <= 15
        ? reinterpret_cast<const char*>(variant.payload)
        : *reinterpret_cast<const char* const*>(variant.payload);
    if (!data && size != 0)
        return false;

    *dataOut = data;
    *sizeOut = size;
    return true;
}

static size_t GetVoiceArgumentCount719(
    const VoiceVariantVector719* arguments) {
    if (!arguments)
        return 0;

    const uintptr_t begin = reinterpret_cast<uintptr_t>(arguments->begin);
    const uintptr_t end = reinterpret_cast<uintptr_t>(arguments->end);
    if (!begin || end < begin || (end - begin) % sizeof(VoiceVariant719) != 0)
        return 0;

    const size_t count = (end - begin) / sizeof(VoiceVariant719);
    return count <= 64 ? count : 0;
}

static bool DecodePublishingOffer719(
    const char* offer, size_t offerSize, std::string& decodedOffer) {
    // Exact 0.719 prefixes compressed SDP with the three-byte marker "pro"
    // and a one-byte compression mode. The remaining bytes are the serialized
    // grpc::SdpOffer message from sdp_compression.proto. Reuse Studio's own
    // codec so its SDP template stays byte-for-byte compatible with Player.
    constexpr size_t kEnvelopeHeaderSize = 4;
    if (!offer || offerSize <= kEnvelopeHeaderSize ||
        std::memcmp(offer, "pro", 3) != 0) {
        return false;
    }

    const int compressionMode = static_cast<unsigned char>(offer[3]);
    const std::string compressedPayload(
        offer + kEnvelopeHeaderSize, offerSize - kEnvelopeHeaderSize);
    const std::uint8_t* imageBase = GetExactStudio719ImageBase();
    if (!imageBase)
        return false;

    auto decompress = reinterpret_cast<types::sdp_decompress_fn>(
        imageBase + kSdpDecompressRva);
    // Message kind 1 is the publishing offer. Kind 2 is the publishing answer
    // consumed by ClientPublishWaitForAnswerState.
    constexpr int kPublishingOffer = 1;
    decodedOffer = decompress(
        compressedPayload, compressionMode, kPublishingOffer);
    if (decodedOffer.empty()) {
        Log("voice",
            "Studio SDP codec returned an empty publishing offer (compressionMode=%d)",
            compressionMode);
        return false;
    }

    Log("voice",
        "Decoded compressed publishing offer with Studio codec mode=%d compressedBytes=%zu sdpBytes=%zu",
        compressionMode, compressedPayload.size(), decodedOffer.size());
    return true;
}

static bool DecodeSubscriptionAnswer719(
    const char* answer, size_t answerSize, std::string& decodedAnswer) {
    // Exact 0.719 uses the same "pro" envelope for subscription answers that
    // it uses for publishing offers. The SDP codec message kinds are ordered
    // publish offer/answer, then subscription offer/answer.
    constexpr size_t kEnvelopeHeaderSize = 4;
    if (!answer || answerSize <= kEnvelopeHeaderSize ||
        std::memcmp(answer, "pro", 3) != 0) {
        return false;
    }

    const int compressionMode = static_cast<unsigned char>(answer[3]);
    const std::string compressedPayload(
        answer + kEnvelopeHeaderSize, answerSize - kEnvelopeHeaderSize);
    const std::uint8_t* imageBase = GetExactStudio719ImageBase();
    if (!imageBase)
        return false;

    auto decompress = reinterpret_cast<types::sdp_decompress_fn>(
        imageBase + kSdpDecompressRva);
    constexpr int kSubscriptionAnswer = 4;
    decodedAnswer = decompress(
        compressedPayload, compressionMode, kSubscriptionAnswer);
    if (decodedAnswer.empty()) {
        Log("voice",
            "Studio SDP codec returned an empty subscription answer (compressionMode=%d)",
            compressionMode);
        return false;
    }

    Log("voice",
        "Decoded compressed subscription answer with Studio codec mode=%d compressedBytes=%zu sdpBytes=%zu",
        compressionMode, compressedPayload.size(), decodedAnswer.size());
    return true;
}

static bool EncodePublishingAnswer719(
    const std::string& plainAnswer, std::string& encodedAnswer) {
    const std::uint8_t* imageBase = GetExactStudio719ImageBase();
    if (!imageBase || plainAnswer.empty())
        return false;

    auto compress = reinterpret_cast<types::sdp_compress_fn>(
        imageBase + kSdpCompressRva);
    auto wrap = reinterpret_cast<types::sdp_wrap_fn>(
        imageBase + kSdpWrapRva);
    constexpr int kProtobufCompressionMode = 2;
    constexpr int kPublishingAnswer = 2;
    types::sdp_compression_result_719 compressed = compress(
        plainAnswer, kProtobufCompressionMode, kPublishingAnswer);
    if (compressed.payload.empty()) {
        Log("voice",
            "Studio SDP codec returned an empty publishing answer (compressionMode=%d)",
            compressed.compression_mode);
        return false;
    }

    encodedAnswer = wrap(compressed.payload, compressed.compression_mode);
    if (encodedAnswer.size() <= 4 ||
        std::memcmp(encodedAnswer.data(), "pro", 3) != 0) {
        Log("voice",
            "Studio SDP envelope rejected publishing answer payloadBytes=%zu mode=%d",
            compressed.payload.size(), compressed.compression_mode);
        encodedAnswer.clear();
        return false;
    }

    Log("voice",
        "Encoded publishing answer with Studio codec mode=%d sdpBytes=%zu compressedBytes=%zu envelopeBytes=%zu",
        compressed.compression_mode, plainAnswer.size(), compressed.payload.size(),
        encodedAnswer.size());
    return true;
}

static bool DecodePublishingHandshakeInitiated719(
    void* rawArguments, std::string& decodedOffer,
    std::string& decodedSessionId, int* muteStateOut = nullptr) {
    const auto* arguments =
        reinterpret_cast<const VoiceVariantVector719*>(rawArguments);
    const size_t argumentCount = GetVoiceArgumentCount719(arguments);
    if (argumentCount != 3) {
        Log("voice", "PublishingHandshakeInitiated argumentCount=%zu (expected 3)",
            argumentCount);
        return false;
    }

    const char* offer = nullptr;
    const char* sessionId = nullptr;
    size_t offerSize = 0;
    size_t sessionIdSize = 0;
    if (!ReadVoiceStringVariant719(
            arguments->begin[0], &offer, &offerSize) ||
        !ReadVoiceStringVariant719(
            arguments->begin[2], &sessionId, &sessionIdSize)) {
        Log("voice", "PublishingHandshakeInitiated contained an invalid string Variant");
        return false;
    }

    const int muteState =
        *reinterpret_cast<const int32_t*>(arguments->begin[1].payload);
    if (muteStateOut)
        *muteStateOut = muteState;
    Log("voice",
        "PublishingHandshakeInitiated decoded muteState=%d sessionId=%.*s offerBytes=%zu",
        muteState, static_cast<int>(sessionIdSize), sessionId, offerSize);

    if (!DecodePublishingOffer719(offer, offerSize, decodedOffer))
        return false;

    decodedSessionId.assign(sessionId, sessionIdSize);
    return true;
}

static bool DecodePublishStateChange719(
    void* rawArguments, int& muteStateOut, std::string& sessionIdOut) {
    const auto* arguments =
        reinterpret_cast<const VoiceVariantVector719*>(rawArguments);
    const size_t argumentCount = GetVoiceArgumentCount719(arguments);
    if (argumentCount != 2) {
        Log("voice", "PublishStateChange argumentCount=%zu (expected 2)",
            argumentCount);
        return false;
    }

    const char* sessionId = nullptr;
    size_t sessionIdSize = 0;
    if (!ReadVoiceStringVariant719(
            arguments->begin[1], &sessionId, &sessionIdSize)) {
        Log("voice", "PublishStateChange contained an invalid session string Variant");
        return false;
    }

    const int muteState =
        *reinterpret_cast<const int32_t*>(arguments->begin[0].payload);
    muteStateOut = muteState;
    sessionIdOut.assign(sessionId, sessionIdSize);
    Log("voice", "PublishStateChange decoded muteState=%d sessionId=%.*s",
        muteState, static_cast<int>(sessionIdSize), sessionId);
    return true;
}

struct RelayCandidatesGathered719 {
    int controlPath = 0;
    bool isLast = false;
    std::string sessionId;
    std::string clientIp;
    int clientPort = 0;
    std::string serializedCandidates;
};

static bool DecodeRelayCandidatesGathered719(
    void* rawArguments, RelayCandidatesGathered719& decoded) {
    const auto* arguments =
        reinterpret_cast<const VoiceVariantVector719*>(rawArguments);
    const size_t argumentCount = GetVoiceArgumentCount719(arguments);
    if (argumentCount != 6) {
        Log("voice", "RelayCandidatesGathered argumentCount=%zu (expected 6)",
            argumentCount);
        return false;
    }

    const char* candidates = nullptr;
    const char* sessionId = nullptr;
    const char* clientIp = nullptr;
    size_t candidatesSize = 0;
    size_t sessionIdSize = 0;
    size_t clientIpSize = 0;
    if (!ReadVoiceStringVariant719(
            arguments->begin[1], &candidates, &candidatesSize) ||
        !ReadVoiceStringVariant719(
            arguments->begin[3], &sessionId, &sessionIdSize) ||
        !ReadVoiceStringVariant719(
            arguments->begin[4], &clientIp, &clientIpSize)) {
        Log("voice", "RelayCandidatesGathered contained an invalid string Variant");
        return false;
    }

    decoded.controlPath =
        *reinterpret_cast<const int32_t*>(arguments->begin[0].payload);
    decoded.isLast = arguments->begin[2].payload[0] != 0;
    decoded.clientPort =
        *reinterpret_cast<const int32_t*>(arguments->begin[5].payload);
    decoded.serializedCandidates.assign(candidates, candidatesSize);
    decoded.sessionId.assign(sessionId, sessionIdSize);
    decoded.clientIp.assign(clientIp, clientIpSize);
    Log("voice",
        "RelayCandidatesGathered decoded controlPath=%d isLast=%u sessionId=%.*s client=%.*s:%d candidateBytes=%zu",
        decoded.controlPath, static_cast<unsigned>(decoded.isLast),
        static_cast<int>(sessionIdSize), sessionId,
        static_cast<int>(clientIpSize), clientIp, decoded.clientPort,
        candidatesSize);
    return true;
}

struct SubscriptionHandshakeAcked719 {
    int64_t eventTag = 0;
    std::string answer;
    std::string usersToMute;
    std::string sessionId;
};

static bool DecodeSubscriptionHandshakeAcked719(
    void* rawArguments, SubscriptionHandshakeAcked719& decoded) {
    const auto* arguments =
        reinterpret_cast<const VoiceVariantVector719*>(rawArguments);
    const size_t argumentCount = GetVoiceArgumentCount719(arguments);
    if (argumentCount != 4) {
        Log("voice", "SubscriptionHandshakeAcked argumentCount=%zu (expected 4)",
            argumentCount);
        return false;
    }

    const char* answer = nullptr;
    const char* usersToMute = nullptr;
    const char* sessionId = nullptr;
    size_t answerSize = 0;
    size_t usersToMuteSize = 0;
    size_t sessionIdSize = 0;
    if (!ReadVoiceStringVariant719(
            arguments->begin[1], &answer, &answerSize) ||
        !ReadVoiceStringVariant719(
            arguments->begin[2], &usersToMute, &usersToMuteSize) ||
        !ReadVoiceStringVariant719(
            arguments->begin[3], &sessionId, &sessionIdSize)) {
        Log("voice",
            "SubscriptionHandshakeAcked contained an invalid string Variant");
        return false;
    }

    decoded.eventTag =
        *reinterpret_cast<const int64_t*>(arguments->begin[0].payload);
    if (answerSize > 4 && std::memcmp(answer, "pro", 3) == 0) {
        if (!DecodeSubscriptionAnswer719(answer, answerSize, decoded.answer))
            return false;
    } else {
        decoded.answer.assign(answer, answerSize);
    }
    decoded.usersToMute.assign(usersToMute, usersToMuteSize);
    decoded.sessionId.assign(sessionId, sessionIdSize);
    Log("voice",
        "SubscriptionHandshakeAcked decoded eventTag=%lld sessionId=%s answerBytes=%zu usersToMuteBytes=%zu",
        static_cast<long long>(decoded.eventTag), decoded.sessionId.c_str(),
        decoded.answer.size(), decoded.usersToMute.size());
    return true;
}

static void LogSubscriptionFeedStarted719(void* rawArguments) {
    const auto* arguments =
        reinterpret_cast<const VoiceVariantVector719*>(rawArguments);
    const size_t argumentCount = GetVoiceArgumentCount719(arguments);
    if (argumentCount != 2) {
        Log("voice", "SubscriptionFeedStarted argumentCount=%zu (expected 2)",
            argumentCount);
        return;
    }

    const char* sessionId = nullptr;
    size_t sessionIdSize = 0;
    if (!ReadVoiceStringVariant719(
            arguments->begin[1], &sessionId, &sessionIdSize)) {
        Log("voice",
            "SubscriptionFeedStarted contained an invalid session string Variant");
        return;
    }
    const int64_t eventTag =
        *reinterpret_cast<const int64_t*>(arguments->begin[0].payload);
    Log("voice",
        "SubscriptionFeedStarted decoded eventTag=%lld sessionId=%.*s",
        static_cast<long long>(eventTag), static_cast<int>(sessionIdSize),
        sessionId);
}

static __int64 ConfigureWithoutRunHook(void* context, uint8_t* settings) {
    if (settings) {
        const uint8_t oldUniverse = settings[kVoiceEnabledForUniverseOnRccOffset];
        const uint8_t oldPlace = settings[kVoiceEnabledForPlaceOnRccOffset];
        bool universeEnabled = true;
        bool placeEnabled = true;
#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
        UniverseVoiceSettings719 coreSettings;
        if (FetchUniverseVoiceSettingsFromCore719(coreSettings)) {
            universeEnabled = coreSettings.universeEnabled;
            placeEnabled = coreSettings.placeEnabled;
        } else {
            Log("voice",
                "Could not fetch the universe voice setting from Core; preserving the enabled compatibility default");
        }
#endif
        settings[kVoiceEnabledForUniverseOnRccOffset] = universeEnabled ? 1 : 0;
        settings[kVoiceEnabledForPlaceOnRccOffset] = placeEnabled ? 1 : 0;
        gVoiceRccConfiguredEnabled.store(
            universeEnabled && placeEnabled, std::memory_order_relaxed);
        const auto* imageBase = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
        const unsigned propertiesFlow = imageBase
            ? imageBase[kVoiceChatEnabledRccProperties2Rva]
            : 0xffu;
        const unsigned controlPlaneFlow = imageBase
            ? imageBase[kVoiceControlPlaneInitFlowWithVoiceTypeRva]
            : 0xffu;
        Log("voice",
            "ConfigureWithoutRun RCC voice gates: universe=%u->%u place=%u->%u propertiesFlow=%u controlPlaneFlow=%u",
            static_cast<unsigned>(oldUniverse),
            static_cast<unsigned>(universeEnabled),
            static_cast<unsigned>(oldPlace),
            static_cast<unsigned>(placeEnabled),
            propertiesFlow, controlPlaneFlow);
    }

    return gOrigConfigureWithoutRun(context, settings);
}

static bool InstallVoiceRccHook() {
    // Unique in exact Studio 0.719.0.7191339. The relative call is wildcarded;
    // the rest anchors the full prologue and settings pointer saved in R15.
    gConfigureWithoutRun = reinterpret_cast<types::configure_without_run_fn>(
        ScanAny("voice.configureWithoutRun", {
            "48 89 5C 24 08 48 89 6C 24 10 56 57 41 54 41 56 41 57 48 83 EC 60 4C 8B FA 4C 8B F1 45 33 E4 44 89 A4 24 A0 00 00 00 E8 ? ? ? ? 48 8B D8",
        }));
    if (!gConfigureWithoutRun) {
        Log("voice", "ConfigureWithoutRun pattern not found -- RCC voice gates remain unchanged");
        return false;
    }

    const MH_STATUS status = MH_CreateHook(
        reinterpret_cast<LPVOID>(gConfigureWithoutRun),
        reinterpret_cast<LPVOID>(&ConfigureWithoutRunHook),
        reinterpret_cast<LPVOID*>(&gOrigConfigureWithoutRun));
    if (status != MH_OK) {
        Log("voice", "MH_CreateHook(ConfigureWithoutRun) failed: %d", static_cast<int>(status));
        gConfigureWithoutRun = nullptr;
        gOrigConfigureWithoutRun = nullptr;
        return false;
    }

    Log("voice", "Hooked ConfigureWithoutRun for 0.719 RCC voice enablement");
    return true;
}

static uint8_t VoiceRccContextGateHook(void* serviceProvider) {
    const void* returnAddress = _ReturnAddress();
    const uint8_t originalResult = gOrigVoiceRccContextGate(serviceProvider);
    const auto* imageBase = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    const uintptr_t returnRva = imageBase && returnAddress
        ? reinterpret_cast<uintptr_t>(returnAddress) - reinterpret_cast<uintptr_t>(imageBase)
        : 0;

    if (returnRva == kConfigureWithoutRunVoiceGateReturnRva) {
        const uint8_t configuredResult = gVoiceRccConfiguredEnabled.load(
            std::memory_order_relaxed) ? 1 : 0;
        Log("voice", "ConfigureWithoutRun context gate: original=%u configured=%u",
            static_cast<unsigned>(originalResult),
            static_cast<unsigned>(configuredResult));
        return configuredResult;
    }

    return originalResult;
}

static bool InstallVoiceRccContextGateHook() {
    // The shared-looking prologue is not unique, so anchor on the exact 0.719
    // tail that checks VoiceChatInternal and DataModel run mode, then recover
    // the function entry. The detour only overrides the ConfigureWithoutRun
    // caller; all other users receive the original result.
    auto* tail = reinterpret_cast<uint8_t*>(ScanAny("voice.contextGate.tail", {
        "49 8B 06 48 85 C0 74 09 80 B8 F1 01 00 00 00 75 09 E8 7A C4 2D FC 84 C0 75 04 B0 01 EB 17 48 8B CE E8 3A 1D B4 01 48 85 C0 74 0A 83 B8 A8 09 00 00 02 0F 94 C0",
    }));
    if (!tail) {
        Log("voice", "RCC context-gate tail pattern not found -- run-mode gate remains unchanged");
        return false;
    }

    gVoiceRccContextGate = reinterpret_cast<types::voice_rcc_context_gate_fn>(
        tail - kVoiceRccContextGateTailOffset);
    const MH_STATUS status = MH_CreateHook(
        reinterpret_cast<LPVOID>(gVoiceRccContextGate),
        reinterpret_cast<LPVOID>(&VoiceRccContextGateHook),
        reinterpret_cast<LPVOID*>(&gOrigVoiceRccContextGate));
    if (status != MH_OK) {
        Log("voice", "MH_CreateHook(contextGate) failed: %d", static_cast<int>(status));
        gVoiceRccContextGate = nullptr;
        gOrigVoiceRccContextGate = nullptr;
        return false;
    }

    Log("voice", "Hooked the 0.719 ConfigureWithoutRun voice context gate @ %p",
        gVoiceRccContextGate);
    return true;
}

static bool SetVoiceStringVariant719(
    VoiceVariant719& variant, void* stringType, void* stringStorageOps,
    const char* value) {
    // MSVC's 0.719 engine string payload is a 16-byte SSO buffer followed by
    // size and capacity. The local identifiers deliberately stay in SSO so no
    // allocation crosses module boundaries.
    const size_t length = std::strlen(value);
    if (length > 15)
        return false;

    std::memset(&variant, 0, sizeof(variant));
    variant.type = stringType;
    variant.storageOps = stringStorageOps;
    std::memcpy(variant.payload, value, length);
    *reinterpret_cast<size_t*>(variant.payload + 16) = length;
    *reinterpret_cast<size_t*>(variant.payload + 24) = 15;
    return true;
}

static void SetOwnedVoiceStringVariant719(
    VoiceVariant719& variant, void* stringType, void* stringStorageOps,
    const std::string& value) {
    // Reflection::Variant stores MSVC std::string inline in its payload. Unlike
    // the short local identifiers, compressed SDP answers require heap-backed
    // storage. LocalRCC's global operator new routes that allocation through
    // Studio's exported Roblox allocator, matching the engine-owned copy made
    // by the synchronous remote-event dispatcher.
    static_assert(sizeof(std::string) <= sizeof(variant.payload));
    std::memset(&variant, 0, sizeof(variant));
    variant.type = stringType;
    variant.storageOps = stringStorageOps;
    new (variant.payload) std::string(value);
}

static void DestroyOwnedVoiceStringVariant719(VoiceVariant719& variant) {
    using VoiceStringStorage719 = std::string;
    reinterpret_cast<VoiceStringStorage719*>(
        variant.payload)->~VoiceStringStorage719();
    std::memset(&variant, 0, sizeof(variant));
}

static bool SendPublishingHandshakeAcked719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& encodedAnswer, const std::string& sessionId) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext ||
        encodedAnswer.empty() || sessionId.empty()) {
        return false;
    }

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!stringType || !stringStorageOps) {
        Log("voice",
            "PublishingHandshakeAcked could not resolve reflected string Variant metadata");
        return false;
    }

    VoiceVariant719 arguments[2] {};
    SetOwnedVoiceStringVariant719(
        arguments[0], stringType, stringStorageOps, encodedAnswer);
    SetOwnedVoiceStringVariant719(
        arguments[1], stringType, stringStorageOps, sessionId);

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 2, arguments + 2};
    VoiceEventTarget719 target {senderContext, 0, 0};
    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice", "PublishingHandshakeAcked found no remote-event dispatcher");
        DestroyOwnedVoiceStringVariant719(arguments[1]);
        DestroyOwnedVoiceStringVariant719(arguments[0]);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService, imageBase + kPublishingHandshakeAckedDescriptorRva,
        &argumentVector, &target);

    DestroyOwnedVoiceStringVariant719(arguments[1]);
    DestroyOwnedVoiceStringVariant719(arguments[0]);
    Log("voice",
        "Sent PublishingHandshakeAcked to senderContext=0x%llx session=%s answerBytes=%zu",
        static_cast<unsigned long long>(senderContext), sessionId.c_str(),
        encodedAnswer.size());
    return true;
}

static bool SendVoiceSessionEvent719(
    const char* eventName, uintptr_t eventDescriptorRva,
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& sessionId) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!eventName || !imageBase || !voiceChatService || !senderContext ||
        sessionId.empty()) {
        return false;
    }

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!stringType || !stringStorageOps) {
        Log("voice",
            "%s could not resolve reflected string Variant metadata",
            eventName);
        return false;
    }

    VoiceVariant719 argument {};
    SetOwnedVoiceStringVariant719(
        argument, stringType, stringStorageOps, sessionId);
    VoiceVariantVector719 argumentVector {
        &argument, &argument + 1, &argument + 1};
    VoiceEventTarget719 target {senderContext, 0, 0};

    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice", "%s found no remote-event dispatcher", eventName);
        DestroyOwnedVoiceStringVariant719(argument);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService, imageBase + eventDescriptorRva,
        &argumentVector, &target);

    DestroyOwnedVoiceStringVariant719(argument);
    Log("voice", "Sent %s to senderContext=0x%llx session=%s", eventName,
        static_cast<unsigned long long>(senderContext), sessionId.c_str());
    return true;
}

static bool SendVoicePlayerMuteStateChanged719(
    uint8_t* voiceChatService, uint64_t senderContext, int muteState) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext)
        return false;

    auto muteStateType = reinterpret_cast<reflected_type_fn>(
        imageBase + kMuteStateVariantTypeRva)();
    if (!muteStateType) {
        Log("voice",
            "VoiceChatPlayerMuteStateChangedServerToClient could not resolve MuteState Variant metadata");
        return false;
    }

    static void* muteStateStorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };
    VoiceVariant719 argument {};
    argument.type = muteStateType;
    argument.storageOps = muteStateStorageOps;
    *reinterpret_cast<int32_t*>(argument.payload) = muteState;

    VoiceVariantVector719 argumentVector {
        &argument, &argument + 1, &argument + 1};
    // This matches VoiceChatService::processRemoteEvent in exact 0.719: mode 1
    // broadcasts the update to every Player except the publishing source while
    // retaining senderContext as the identity of the player whose state changed.
    VoiceEventTarget719 target {senderContext, 1, 0};
    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice",
            "VoiceChatPlayerMuteStateChangedServerToClient found no remote-event dispatcher");
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService,
        imageBase + kVoicePlayerMuteStateChangedServerToClientDescriptorRva,
        &argumentVector, &target);
    Log("voice",
        "Broadcast VoiceChatPlayerMuteStateChangedServerToClient senderContext=0x%llx muteState=%d",
        static_cast<unsigned long long>(senderContext), muteState);
    return true;
}

static bool SendVoicePlayerMuteStatusChanged719(
    uint8_t* voiceChatService, uint64_t recipientSenderContext,
    int64_t userId, int muteState, const std::string& sessionId) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !recipientSenderContext ||
        userId <= 0 || sessionId.empty()) {
        return false;
    }

    auto int64Type = reinterpret_cast<reflected_type_fn>(
        imageBase + kInt64VariantTypeRva)();
    auto boolType = reinterpret_cast<reflected_type_fn>(
        imageBase + kBoolVariantTypeRva)();
    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!int64Type || !boolType || !stringType || !stringStorageOps) {
        Log("voice",
            "VoiceChatplayerMuteStatusChangedEvent could not resolve Variant metadata");
        return false;
    }

    static void* trivialStorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };
    VoiceVariant719 arguments[3] {};
    arguments[0].type = int64Type;
    arguments[0].storageOps = trivialStorageOps;
    *reinterpret_cast<int64_t*>(arguments[0].payload) = userId;
    arguments[1].type = boolType;
    arguments[1].storageOps = trivialStorageOps;
    arguments[1].payload[0] = muteState != 0;
    SetOwnedVoiceStringVariant719(
        arguments[2], stringType, stringStorageOps, sessionId);

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 3, arguments + 3};
    VoiceEventTarget719 target {recipientSenderContext, 0, 0};
    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice",
            "VoiceChatplayerMuteStatusChangedEvent found no remote-event dispatcher");
        DestroyOwnedVoiceStringVariant719(arguments[2]);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService,
        imageBase + kVoicePlayerMuteStatusChangedDescriptorRva,
        &argumentVector, &target);
    DestroyOwnedVoiceStringVariant719(arguments[2]);
    Log("voice",
        "Sent VoiceChatplayerMuteStatusChangedEvent recipient=0x%llx userId=%lld muted=%d session=%s",
        static_cast<unsigned long long>(recipientSenderContext),
        static_cast<long long>(userId), muteState != 0, sessionId.c_str());
    return true;
}

static bool SendPublishingHandshakeCompleted719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& sessionId) {
    return SendVoiceSessionEvent719(
        "PublishingHandshakeCompleted",
        kPublishingHandshakeCompletedDescriptorRva,
        voiceChatService, senderContext, sessionId);
}

static bool SendVoiceSubscriptionInitialBatchEmpty719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& sessionId) {
    return SendVoiceSessionEvent719(
        "VoiceChatSubscriptionInitialBatchEmpty",
        kVoiceSubscriptionInitialBatchEmptyDescriptorRva,
        voiceChatService, senderContext, sessionId);
}

static bool SendSubscriptionHandshakeInitiated719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& offer, const std::string& subscriptionState,
    bool isNewConnection, const std::string& sessionId, int64_t eventTag) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext || offer.empty() ||
        subscriptionState.empty() || sessionId.empty()) {
        return false;
    }

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto boolType = reinterpret_cast<reflected_type_fn>(
        imageBase + kBoolVariantTypeRva)();
    auto int64Type = reinterpret_cast<reflected_type_fn>(
        imageBase + kInt64VariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!stringType || !boolType || !int64Type || !stringStorageOps) {
        Log("voice",
            "SubscriptionHandshakeInitiated could not resolve reflected Variant metadata");
        return false;
    }

    static void* trivialStorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };
    VoiceVariant719 arguments[5] {};
    SetOwnedVoiceStringVariant719(
        arguments[0], stringType, stringStorageOps, offer);
    SetOwnedVoiceStringVariant719(
        arguments[1], stringType, stringStorageOps, subscriptionState);
    arguments[2].type = boolType;
    arguments[2].storageOps = trivialStorageOps;
    arguments[2].payload[0] = isNewConnection ? 1 : 0;
    SetOwnedVoiceStringVariant719(
        arguments[3], stringType, stringStorageOps, sessionId);
    arguments[4].type = int64Type;
    arguments[4].storageOps = trivialStorageOps;
    *reinterpret_cast<int64_t*>(arguments[4].payload) = eventTag;

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 5, arguments + 5};
    VoiceEventTarget719 target {senderContext, 0, 0};
    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice",
            "SubscriptionHandshakeInitiated found no remote-event dispatcher");
        DestroyOwnedVoiceStringVariant719(arguments[3]);
        DestroyOwnedVoiceStringVariant719(arguments[1]);
        DestroyOwnedVoiceStringVariant719(arguments[0]);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService,
        imageBase + kSubscriptionHandshakeInitiatedDescriptorRva,
        &argumentVector, &target);

    DestroyOwnedVoiceStringVariant719(arguments[3]);
    DestroyOwnedVoiceStringVariant719(arguments[1]);
    DestroyOwnedVoiceStringVariant719(arguments[0]);
    Log("voice",
        "Sent SubscriptionHandshakeInitiated senderContext=0x%llx session=%s eventTag=%lld newConnection=%u offerBytes=%zu stateBytes=%zu",
        static_cast<unsigned long long>(senderContext), sessionId.c_str(),
        static_cast<long long>(eventTag),
        static_cast<unsigned>(isNewConnection), offer.size(),
        subscriptionState.size());
    return true;
}

static bool SendSubscriptionHandshakeCompleted719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& sessionId, int64_t eventTag) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext || sessionId.empty())
        return false;

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto int64Type = reinterpret_cast<reflected_type_fn>(
        imageBase + kInt64VariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!stringType || !int64Type || !stringStorageOps) {
        Log("voice",
            "SubscriptionHandshakeCompleted could not resolve reflected Variant metadata");
        return false;
    }

    static void* int64StorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };
    VoiceVariant719 arguments[2] {};
    SetOwnedVoiceStringVariant719(
        arguments[0], stringType, stringStorageOps, sessionId);
    arguments[1].type = int64Type;
    arguments[1].storageOps = int64StorageOps;
    *reinterpret_cast<int64_t*>(arguments[1].payload) = eventTag;

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 2, arguments + 2};
    VoiceEventTarget719 target {senderContext, 0, 0};
    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice",
            "SubscriptionHandshakeCompleted found no remote-event dispatcher");
        DestroyOwnedVoiceStringVariant719(arguments[0]);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService,
        imageBase + kSubscriptionHandshakeCompletedDescriptorRva,
        &argumentVector, &target);

    DestroyOwnedVoiceStringVariant719(arguments[0]);
    Log("voice",
        "Sent SubscriptionHandshakeCompleted senderContext=0x%llx session=%s eventTag=%lld",
        static_cast<unsigned long long>(senderContext), sessionId.c_str(),
        static_cast<long long>(eventTag));
    return true;
}

struct VoiceParticipantIdentity719 {
    uint64_t senderContext = 0;
    int64_t participantId = 0;
    int64_t userId = 0;
    std::string sessionId;
};

static std::mutex gVoiceParticipantIdentityMutex719;
static std::vector<VoiceParticipantIdentity719> gVoiceParticipantIdentities719;
static int64_t gNextVoiceSessionOrdinal719 = 1;

static bool IsReadableVoiceMemory719(const void* address, size_t size) {
    if (!address || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION region {};
    if (VirtualQuery(address, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        region.RegionSize < size) {
        return false;
    }

    const uintptr_t readAddress = reinterpret_cast<uintptr_t>(address);
    const uintptr_t regionAddress =
        reinterpret_cast<uintptr_t>(region.BaseAddress);
    return readAddress >= regionAddress &&
        readAddress - regionAddress <= region.RegionSize - size;
}

static int64_t ResolveVoiceUserId719(uint64_t senderContext) {
    if (senderContext < static_cast<uint64_t>(kVoiceSenderContextOffset))
        return 0;

    auto* serverReplicator = reinterpret_cast<uint8_t*>(
        static_cast<uintptr_t>(senderContext - kVoiceSenderContextOffset));
    uint8_t** remotePlayerAddress = reinterpret_cast<uint8_t**>(
        serverReplicator + kServerReplicatorRemotePlayerOffset);
    if (!IsReadableVoiceMemory719(remotePlayerAddress, sizeof(*remotePlayerAddress))) {
        Log("voice",
            "Could not read ServerReplicator remote Player senderContext=0x%llx replicator=%p",
            static_cast<unsigned long long>(senderContext), serverReplicator);
        return 0;
    }

    uint8_t* remotePlayer = nullptr;
    std::memcpy(&remotePlayer, remotePlayerAddress, sizeof(remotePlayer));
    const uint8_t* userIdAddress = remotePlayer
        ? remotePlayer + kPlayerUserIdOffset
        : nullptr;
    const uint8_t* encodedUserIdAddress = remotePlayer
        ? remotePlayer + kPlayerEncodedUserIdOffset
        : nullptr;
    if (!IsReadableVoiceMemory719(userIdAddress, sizeof(int64_t)) ||
        !IsReadableVoiceMemory719(encodedUserIdAddress, sizeof(int64_t))) {
        Log("voice",
            "ServerReplicator has no readable remote Player identity senderContext=0x%llx replicator=%p player=%p",
            static_cast<unsigned long long>(senderContext), serverReplicator,
            remotePlayer);
        return 0;
    }

    int64_t userId = 0;
    uint64_t encodedUserId = 0;
    std::memcpy(&userId, userIdAddress, sizeof(userId));
    std::memcpy(&encodedUserId, encodedUserIdAddress, sizeof(encodedUserId));
    if (static_cast<uint64_t>(userId) !=
            encodedUserId - kPlayerUserIdEncodingBias ||
        userId <= 0) {
        Log("voice",
            "Rejected remote Player identity senderContext=0x%llx player=%p userId=%lld encoded=0x%llx",
            static_cast<unsigned long long>(senderContext), remotePlayer,
            static_cast<long long>(userId),
            static_cast<unsigned long long>(encodedUserId));
        return 0;
    }

    Log("voice",
        "Resolved remote Player voice identity senderContext=0x%llx replicator=%p player=%p userId=%lld",
        static_cast<unsigned long long>(senderContext), serverReplicator,
        remotePlayer, static_cast<long long>(userId));
    return userId;
}

static VoiceParticipantIdentity719 GetOrCreateVoiceParticipantIdentity719(
    uint64_t senderContext) {
    std::lock_guard<std::mutex> lock(gVoiceParticipantIdentityMutex719);
    for (const VoiceParticipantIdentity719& identity :
         gVoiceParticipantIdentities719) {
        if (identity.senderContext == senderContext)
            return identity;
    }

    VoiceParticipantIdentity719 identity;
    identity.senderContext = senderContext;
    const int64_t sessionOrdinal = gNextVoiceSessionOrdinal719++;
    identity.participantId = sessionOrdinal;
    identity.userId = ResolveVoiceUserId719(senderContext);
    if (identity.userId <= 0) {
        identity.userId = identity.participantId;
        Log("voice",
            "Falling back to local subscription userId=%lld senderContext=0x%llx",
            static_cast<long long>(identity.userId),
            static_cast<unsigned long long>(senderContext));
    }
    if (sessionOrdinal == 1) {
        // Preserve the identifier used by the already validated one-player
        // path. Later team-test Players need distinct session identifiers so
        // their publishing and subscription ICE cannot cross over.
        identity.sessionId = "local-session";
    } else {
        char sessionId[16] {};
        std::snprintf(sessionId, sizeof(sessionId), "local-%08llx",
            static_cast<unsigned long long>(sessionOrdinal));
        identity.sessionId = sessionId;
    }
    gVoiceParticipantIdentities719.push_back(identity);
    Log("voice",
        "Assigned team-test voice participantId=%lld userId=%lld session=%s senderContext=0x%llx",
        static_cast<long long>(identity.participantId),
        static_cast<long long>(identity.userId),
        identity.sessionId.c_str(),
        static_cast<unsigned long long>(senderContext));
    return identity;
}

static VoiceParticipantIdentity719 RotateVoiceParticipantIdentity719(
    uint64_t senderContext) {
    std::lock_guard<std::mutex> lock(gVoiceParticipantIdentityMutex719);

    int64_t userId = 0;
    for (const VoiceParticipantIdentity719& identity :
         gVoiceParticipantIdentities719) {
        if (identity.senderContext == senderContext) {
            userId = identity.userId;
            break;
        }
    }
    if (userId <= 0)
        userId = ResolveVoiceUserId719(senderContext);

    const int64_t sessionOrdinal = gNextVoiceSessionOrdinal719++;
    VoiceParticipantIdentity719 replacement;
    replacement.senderContext = senderContext;
    replacement.participantId = sessionOrdinal;
    replacement.userId = userId > 0 ? userId : sessionOrdinal;
    char sessionId[16] {};
    std::snprintf(sessionId, sizeof(sessionId), "local-%08llx",
        static_cast<unsigned long long>(sessionOrdinal));
    replacement.sessionId = sessionId;

    bool replaced = false;
    for (VoiceParticipantIdentity719& identity :
         gVoiceParticipantIdentities719) {
        if (identity.senderContext == senderContext) {
            identity = replacement;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        gVoiceParticipantIdentities719.push_back(replacement);

    Log("voice",
        "Rotated rejoin identity participantId=%lld userId=%lld session=%s senderContext=0x%llx",
        static_cast<long long>(replacement.participantId),
        static_cast<long long>(replacement.userId), replacement.sessionId.c_str(),
        static_cast<unsigned long long>(senderContext));
    return replacement;
}

static bool SendJoinedVoice719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const VoiceParticipantIdentity719& identity, bool rejoining = false) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext)
        return false;

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto int64Type = reinterpret_cast<reflected_type_fn>(
        imageBase + kInt64VariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    if (!stringType || !int64Type || !stringStorageOps) {
        Log("voice", "JoinedVoice could not resolve reflected Variant metadata");
        return false;
    }

    // The int64 payload is trivially copied and destroyed. This is the same
    // three-entry operation table that 0.719 lazily creates for int64 Variants,
    // but kept in LocalRCC so we do not mutate the engine's function-static
    // initialization state.
    static void* int64StorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };

    VoiceVariant719 arguments[4] {};
    if (!SetVoiceStringVariant719(
            arguments[0], stringType, stringStorageOps, "local-teamtest") ||
        !SetVoiceStringVariant719(
            arguments[2], stringType, stringStorageOps,
            identity.sessionId.c_str()) ||
        !SetVoiceStringVariant719(
            arguments[3], stringType, stringStorageOps, "local-channel")) {
        Log("voice", "JoinedVoice identifier exceeded the 0.719 SSO limit");
        return false;
    }

    arguments[1].type = int64Type;
    arguments[1].storageOps = int64StorageOps;
    *reinterpret_cast<int64_t*>(arguments[1].payload) = identity.participantId;

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 4, arguments + 4};
    // EventInvocationSignalData target mode 0 selects the Replicator identified
    // by senderContext. Mode 1 is the broadcast-except form used by the native
    // mute propagation path; using it for JoinedVoice excluded the requesting
    // Player and produced no outgoing EventInvocationItem in a one-client test.
    VoiceEventTarget719 target {senderContext, 0, 0};

    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice", "JoinedVoice found no remote-event dispatcher");
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService,
        imageBase + (rejoining
            ? kReJoinedVoiceDescriptorRva
            : kJoinedVoiceDescriptorRva),
        &argumentVector, &target);
    Log("voice",
        "Sent %s to senderContext=0x%llx channel=local-teamtest participantId=%lld",
        rejoining ? "ReJoinedVoice" : "JoinedVoice",
        static_cast<unsigned long long>(senderContext),
        static_cast<long long>(identity.participantId));
    return true;
}

#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
struct TurnAuthResponse719 {
    std::vector<std::string> uris;
    std::string username;
    std::string password;
    int ttlSeconds = 3600;
};

static bool FetchJsonFromCore719(
    const wchar_t* path, const wchar_t* additionalHeaders,
    nlohmann::json& document) {
    if (!path)
        return false;

    char portText[16] {};
    const DWORD portLength = GetEnvironmentVariableA(
        "NOOBHOOK_HTTP_PORT", portText, sizeof(portText));
    unsigned long parsedPort = 8080;
    if (portLength > 0 && portLength < sizeof(portText)) {
        char* end = nullptr;
        const unsigned long value = std::strtoul(portText, &end, 10);
        if (end && *end == '\0' && value > 0 && value <= 65535)
            parsedPort = value;
    }

    HINTERNET internet = WinHttpOpen(
        L"noobWarrior LocalRCC voice/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!internet)
        return false;
    WinHttpSetTimeouts(internet, 1500, 1500, 1500, 2000);

    HINTERNET connection = WinHttpConnect(
        internet, L"127.0.0.1", static_cast<INTERNET_PORT>(parsedPort), 0);
    if (!connection) {
        WinHttpCloseHandle(internet);
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(
        connection, L"GET", path, nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(internet);
        return false;
    }

    bool success = additionalHeaders == nullptr || WinHttpAddRequestHeaders(
        request, additionalHeaders, static_cast<DWORD>(-1L),
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) != FALSE;
    success = success && WinHttpSendRequest(
        request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE &&
        WinHttpReceiveResponse(request, nullptr) != FALSE;
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    success = success && WinHttpQueryHeaders(
        request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
        WINHTTP_NO_HEADER_INDEX) != FALSE && status == 200;

    std::string response;
    while (success) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            success = false;
            break;
        }
        if (available == 0)
            break;
        if (available > 16384 || response.size() > 16384 - available) {
            success = false;
            break;
        }
        const size_t offset = response.size();
        response.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(
                request, response.data() + offset, available, &read)) {
            success = false;
            break;
        }
        response.resize(offset + read);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(internet);
    if (!success || response.empty())
        return false;

    try {
        document = nlohmann::json::parse(response);
        return document.is_object();
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

static bool FetchUniverseVoiceSettingsFromCore719(
    UniverseVoiceSettings719& settings) {
    char placeText[32] {};
    const DWORD placeLength = GetEnvironmentVariableA(
        "NOOBHOOK_PLACEID", placeText, sizeof(placeText));
    if (placeLength == 0 || placeLength >= sizeof(placeText))
        return false;

    char* end = nullptr;
    const long long placeId = std::strtoll(placeText, &end, 10);
    if (!end || *end != '\0' || placeId <= 0)
        return false;

    const std::wstring placeHeader = L"Roblox-Place-Id: " +
        std::to_wstring(placeId) + L"\r\n";
    nlohmann::json document;
    if (!FetchJsonFromCore719(
            L"/v2/rccsettings/universe", placeHeader.c_str(), document)) {
        return false;
    }

    try {
        if (!document.contains("isUniverseEnabledForVoice") ||
            !document["isUniverseEnabledForVoice"].is_boolean() ||
            !document.contains("isPlaceEnabledForVoice") ||
            !document["isPlaceEnabledForVoice"].is_boolean()) {
            return false;
        }
        settings.universeEnabled =
            document["isUniverseEnabledForVoice"].get<bool>();
        settings.placeEnabled =
            document["isPlaceEnabledForVoice"].get<bool>();
    } catch (const nlohmann::json::exception&) {
        return false;
    }

    Log("voice",
        "Fetched universe voice settings from local Core placeId=%lld universe=%u place=%u",
        placeId, static_cast<unsigned>(settings.universeEnabled),
        static_cast<unsigned>(settings.placeEnabled));
    return true;
}

static bool FetchTurnAuthFromCore719(TurnAuthResponse719& auth) {
    nlohmann::json document;
    if (!FetchJsonFromCore719(
            L"/emu/v1/voice/turn-auth", nullptr, document)) {
        return false;
    }

    try {
        if (!document.contains("uris") || !document["uris"].is_array() ||
            document["uris"].empty() || document["uris"].size() > 4) {
            return false;
        }
        auth.uris.clear();
        for (const nlohmann::json& uriValue : document["uris"]) {
            if (!uriValue.is_string())
                return false;
            std::string uri = uriValue.get<std::string>();
            if (!uri.starts_with("turn:") || uri.size() > 1024)
                return false;
            auth.uris.push_back(std::move(uri));
        }
        auth.username = document.value("username", "");
        auth.password = document.value("password", "");
        auth.ttlSeconds = document.value("ttl", 3600);
        if (auth.uris.empty() || auth.username.empty() ||
            auth.password.empty() || auth.ttlSeconds < 60 ||
            auth.ttlSeconds > 86400) {
            return false;
        }
    } catch (const nlohmann::json::exception&) {
        return false;
    }

    Log("voice",
        "Fetched short-lived TURN auth from local Core primaryUri=%s uriCount=%zu ttl=%d",
        auth.uris.front().c_str(), auth.uris.size(), auth.ttlSeconds);
    return true;
}
#endif

static bool SendUserTurnAuth719(
    uint8_t* voiceChatService, uint64_t senderContext,
    const std::string& sessionId) {
    auto* imageBase = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!imageBase || !voiceChatService || !senderContext)
        return false;

    auto stringType = reinterpret_cast<reflected_type_fn>(
        imageBase + kStringVariantTypeRva)();
    auto intType = reinterpret_cast<reflected_type_fn>(
        imageBase + kIntVariantTypeRva)();
    auto arrayType = reinterpret_cast<reflected_type_fn>(
        imageBase + kArrayVariantTypeRva)();
    auto stringStorageOps = reinterpret_cast<reflected_storage_ops_fn>(
        imageBase + kStringVariantStorageOpsRva)();
    auto assignArrayStorage =
        reinterpret_cast<assign_array_variant_storage_fn>(
            imageBase + kAssignArrayVariantStorageRva);
    if (!stringType || !intType || !arrayType || !stringStorageOps ||
        !assignArrayStorage) {
        Log("voice", "UserTurnAuth could not resolve reflected Variant metadata");
        return false;
    }

    std::string turnUserName = "local-turn";
    std::string turnPassword = "local-pass";
    std::vector<std::string> turnUris;
    int ttlSeconds = 3600;
#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
    TurnAuthResponse719 coreAuth;
    if (FetchTurnAuthFromCore719(coreAuth)) {
        turnUserName = std::move(coreAuth.username);
        turnPassword = std::move(coreAuth.password);
        turnUris = std::move(coreAuth.uris);
        ttlSeconds = coreAuth.ttlSeconds;
    }
#endif

    // Keep a LAN fallback for older Core builds and startup failures. The
    // Windows 0.719 Player has no factory option for loopback ICE networks.
    auto findTurnIpv4Address = []() -> std::string {
        ULONG bufferSize = 16 * 1024;
        std::vector<unsigned char> buffer(bufferSize);
        constexpr ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
            GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_GATEWAYS;
        ULONG result = GetAdaptersAddresses(
            AF_INET, flags, nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &bufferSize);
        if (result == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufferSize);
            result = GetAdaptersAddresses(
                AF_INET, flags, nullptr,
                reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &bufferSize);
        }
        if (result != NO_ERROR)
            return {};

        auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        for (const bool requireGateway : {true, false}) {
            for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
                if (adapter->OperStatus != IfOperStatusUp ||
                    adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
                    adapter->IfType == IF_TYPE_TUNNEL ||
                    (requireGateway && adapter->FirstGatewayAddress == nullptr)) {
                    continue;
                }
                for (auto* unicast = adapter->FirstUnicastAddress;
                     unicast != nullptr; unicast = unicast->Next) {
                    if (!unicast->Address.lpSockaddr ||
                        unicast->Address.lpSockaddr->sa_family != AF_INET) {
                        continue;
                    }
                    const auto* address = reinterpret_cast<const sockaddr_in*>(
                        unicast->Address.lpSockaddr);
                    const auto* octets = reinterpret_cast<const unsigned char*>(
                        &address->sin_addr.S_un.S_addr);
                    if (octets[0] == 0 || octets[0] == 127 || octets[0] >= 224 ||
                        (octets[0] == 169 && octets[1] == 254)) {
                        continue;
                    }
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
    };
    if (turnUris.empty()) {
        const std::string turnHost = findTurnIpv4Address();
        if (turnHost.empty()) {
            Log("voice", "UserTurnAuth found no usable non-loopback IPv4 interface");
            return false;
        }
        turnUris.push_back(
            "turn:" + turnHost + ":3478?transport=udp");
        Log("voice", "Using LAN TURN fallback because Core auth was unavailable");
    }

    VoiceVariant719 arguments[5] {};
    SetOwnedVoiceStringVariant719(
        arguments[0], stringType, stringStorageOps, sessionId);
    SetOwnedVoiceStringVariant719(
        arguments[1], stringType, stringStorageOps, turnUserName);
    SetOwnedVoiceStringVariant719(
        arguments[2], stringType, stringStorageOps, turnPassword);

    // int and int64 Variants use the same trivial copy/no-op destruction shape.
    // The copy routine moves eight zero-padded payload bytes; the reflected int
    // type controls how the serializer encodes the value.
    static void* intStorageOps[3] {
        imageBase + kInt64VariantCopyRva,
        imageBase + kInt64VariantCopyRva,
        imageBase + kVariantNoopDestructorRva,
    };
    arguments[3].type = intType;
    arguments[3].storageOps = intStorageOps;
    *reinterpret_cast<int32_t*>(arguments[3].payload) = ttlSeconds;

    // Reflection::ValueArray is a shared vector of Variants. Let the engine's
    // exact 0.719 assignment helper install its storage-operation table and
    // retain the MSVC-compatible shared_ptr in the Variant payload.
    using VoiceValueArray719 = std::vector<VoiceVariant719>;
    auto turnUriVariants = std::shared_ptr<VoiceValueArray719>(
        new VoiceValueArray719(), [](VoiceValueArray719* uris) {
            // VoiceVariant719 is intentionally a POD ABI mirror, so its vector
            // does not invoke Reflection::Variant's destructor. Release each
            // placement-constructed URI string when the last engine/local
            // shared_ptr drops the ValueArray after serialization.
            if (uris) {
                for (VoiceVariant719& uri : *uris)
                    DestroyOwnedVoiceStringVariant719(uri);
            }
            delete uris;
        });
    turnUriVariants->reserve(turnUris.size());
    for (const std::string& turnUri : turnUris) {
        turnUriVariants->emplace_back();
        SetOwnedVoiceStringVariant719(
            turnUriVariants->back(), stringType, stringStorageOps, turnUri);
    }
    arguments[4].type = arrayType;
    assignArrayStorage(&arguments[4].storageOps, &turnUriVariants);
    if (!arguments[4].storageOps) {
        Log("voice", "UserTurnAuth could not initialize the URI array Variant");
        DestroyOwnedVoiceStringVariant719(arguments[2]);
        DestroyOwnedVoiceStringVariant719(arguments[1]);
        DestroyOwnedVoiceStringVariant719(arguments[0]);
        return false;
    }

    VoiceVariantVector719 argumentVector {
        arguments, arguments + 5, arguments + 5};
    VoiceEventTarget719 target {senderContext, 0, 0};

    auto** vtable = *reinterpret_cast<void***>(voiceChatService);
    if (!vtable || !vtable[3]) {
        Log("voice", "UserTurnAuth found no remote-event dispatcher");
        std::shared_ptr<VoiceValueArray719> emptyUris;
        assignArrayStorage(&arguments[4].storageOps, &emptyUris);
        DestroyOwnedVoiceStringVariant719(arguments[2]);
        DestroyOwnedVoiceStringVariant719(arguments[1]);
        DestroyOwnedVoiceStringVariant719(arguments[0]);
        return false;
    }

    auto sendRemoteEvent = reinterpret_cast<send_remote_event_fn>(vtable[3]);
    sendRemoteEvent(
        voiceChatService, imageBase + kUserTurnAuthDescriptorRva,
        &argumentVector, &target);

    // VoiceVariant719 is intentionally POD, so explicitly release the
    // shared_ptr retained by the engine assignment helper after dispatch has
    // synchronously copied the argument vector.
    std::shared_ptr<VoiceValueArray719> emptyUris;
    assignArrayStorage(&arguments[4].storageOps, &emptyUris);
    DestroyOwnedVoiceStringVariant719(arguments[2]);
    DestroyOwnedVoiceStringVariant719(arguments[1]);
    DestroyOwnedVoiceStringVariant719(arguments[0]);

    Log("voice",
        "Sent UserTurnAuth to senderContext=0x%llx session=%s primaryUri=%s uriCount=%zu ttl=%d",
        static_cast<unsigned long long>(senderContext), sessionId.c_str(),
        turnUris.front().c_str(), turnUris.size(),
        ttlSeconds);
    return true;
}

#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
struct VoicePublishingPeer719 {
    int peer = -1;
    std::string sessionId;
    std::string decodedOffer;
    uint8_t* voiceChatService = nullptr;
    uint64_t senderContext = 0;
    int64_t participantId = 0;
    int64_t userId = 0;
    int64_t feedId = 0;
    uint32_t ssrc = 0;
    std::atomic<int> muteState {1};
    std::atomic<int> incomingTrack {-1};
    std::mutex stateMutex;
    std::condition_variable stateChanged;
    bool gatheringComplete = false;
    bool failed = false;
    // 0 = pending, 1 = dispatching, 2 = dispatched. The media callback can be
    // entered from libdatachannel worker threads, so do not expose completion
    // to the dependent initial-batch event until dispatch has returned.
    std::atomic<uint8_t> publishingCompletionState {0};
    std::atomic<bool> subscriptionInitialBatchEmptySent {false};
    std::atomic<uint64_t> receivedPackets {0};
};

struct VoiceSubscriptionRoute719 {
    uint64_t sourceSenderContext = 0;
    int64_t userId = 0;
    int64_t feedId = 0;
    uint32_t ssrc = 0;
    int track = -1;
    int midExtensionId = -1;
    std::string sourceMid;
    std::string mid;
    std::string targetSessionId;
    std::atomic<uint64_t> forwardedPackets {0};
    std::atomic<uint64_t> droppedPackets {0};
    std::atomic<uint64_t> feedbackPackets {0};

    VoiceSubscriptionRoute719() = default;
    VoiceSubscriptionRoute719(const VoiceSubscriptionRoute719&) = delete;
    VoiceSubscriptionRoute719& operator=(
        const VoiceSubscriptionRoute719&) = delete;
};

struct VoiceSubscriptionPeer719 {
    int peer = -1;
    std::string sessionId;
    uint8_t* voiceChatService = nullptr;
    uint64_t senderContext = 0;
    int64_t eventTag = 0;
    std::vector<std::unique_ptr<VoiceSubscriptionRoute719>> routes;
    std::mutex stateMutex;
    std::condition_variable stateChanged;
    bool gatheringComplete = false;
    bool failed = false;
    std::atomic<bool> answerApplied {false};
    std::atomic<bool> negotiationPending {false};
    std::atomic<bool> connected {false};
    std::atomic<bool> completionSent {false};
};

// The exact 0.719 callbacks and native media tracks live inside Studio, so the
// publishing/subscription peer registry stays process-local. Core supplies the
// emulator and TURN reachability; moving live track callbacks across an IPC
// boundary would add complexity without improving the LAN relay.
static std::mutex gVoicePeerMutex719;
// Process-lifetime owner. Deliberately do not run libdatachannel teardown from
// a C++ global destructor: DLL destructors execute under the Windows loader
// lock, where closing a peer and joining its worker threads can deadlock. A
// replacement handshake explicitly closes the previous peer; process exit lets
// Windows reclaim the final one.
static std::vector<VoicePublishingPeer719*> gVoicePublishingPeers719;
static std::vector<VoicePublishingPeer719*> gRetiredVoicePublishingPeers719;
static std::vector<VoiceSubscriptionPeer719*> gVoiceSubscriptionPeers719;
static std::vector<VoiceSubscriptionPeer719*> gRetiredVoiceSubscriptionPeers719;
static std::atomic<int64_t> gNextVoiceSubscriptionEventTag719 {1};

static void HandleRelayCandidatesGathered719(
    void* rawArguments, uint64_t senderContext) {
    RelayCandidatesGathered719 decoded;
    if (!DecodeRelayCandidatesGathered719(rawArguments, decoded))
        return;

    // Voice::ControlPath::Publish is zero and Subscription is one in exact
    // 0.719. Each Player owns one publishing transport and, once another
    // participant exists, one subscription transport.
    if (decoded.controlPath != 0 && decoded.controlPath != 1) {
        Log("voice",
            "Ignoring RelayCandidatesGathered for unsupported controlPath=%d",
            decoded.controlPath);
        return;
    }

    const nlohmann::json serialized = nlohmann::json::parse(
        decoded.serializedCandidates, nullptr, false);
    if (serialized.is_discarded()) {
        Log("voice", "RelayCandidatesGathered contained invalid candidate JSON");
        return;
    }

    int peer = -1;
    {
        std::lock_guard<std::mutex> peerLock(gVoicePeerMutex719);
        if (decoded.controlPath == 0) {
            for (VoicePublishingPeer719* candidate :
                 gVoicePublishingPeers719) {
                if (candidate && candidate->senderContext == senderContext &&
                    candidate->sessionId == decoded.sessionId) {
                    peer = candidate->peer;
                    break;
                }
            }
        } else {
            for (VoiceSubscriptionPeer719* candidate :
                 gVoiceSubscriptionPeers719) {
                if (candidate && candidate->senderContext == senderContext &&
                    candidate->sessionId == decoded.sessionId) {
                    peer = candidate->peer;
                    break;
                }
            }
        }
    }
    if (peer < 0) {
        Log("voice",
            "RelayCandidatesGathered has no active %s peer senderContext=0x%llx session=%s",
            decoded.controlPath == 0 ? "publishing" : "subscription",
            static_cast<unsigned long long>(senderContext),
            decoded.sessionId.c_str());
        return;
    }

    size_t candidateCount = 0;
    size_t acceptedCount = 0;
    auto addCandidate = [&](const nlohmann::json& entry) {
        if (!entry.is_object())
            return;

        const auto candidateIt = entry.find("candidate");
        if (candidateIt == entry.end() || !candidateIt->is_string() ||
            candidateIt->get_ref<const std::string&>().empty()) {
            return;
        }

        ++candidateCount;
        std::string candidate = candidateIt->get<std::string>();
        if (candidate.starts_with("a="))
            candidate.erase(0, 2);

        std::string mid = "0";
        const auto midIt = entry.find("sdpMid");
        if (midIt != entry.end() && midIt->is_string() &&
            !midIt->get_ref<const std::string&>().empty()) {
            mid = midIt->get<std::string>();
        }

        const int result = rtcAddRemoteCandidate(
            peer, candidate.c_str(), mid.c_str());
        Log("voice",
            "Applied Player %s ICE candidate peer=%d mid=%s result=%d",
            decoded.controlPath == 0 ? "publishing" : "subscription",
            peer, mid.c_str(), result);
        if (result >= 0)
            ++acceptedCount;
    };

    if (serialized.is_array()) {
        for (const nlohmann::json& candidate : serialized)
            addCandidate(candidate);
    } else if (serialized.is_object()) {
        const auto candidatesIt = serialized.find("candidates");
        if (candidatesIt != serialized.end() && candidatesIt->is_array()) {
            for (const nlohmann::json& candidate : *candidatesIt)
                addCandidate(candidate);
        } else {
            // Accept a single candidate object as well as the engine's normal
            // {"candidates":[...]} envelope so the bridge is version-tolerant.
            addCandidate(serialized);
        }
    }

    Log("voice",
        "Processed Player %s ICE candidates session=%s parsed=%zu accepted=%zu isLast=%u",
        decoded.controlPath == 0 ? "publishing" : "subscription",
        decoded.sessionId.c_str(), candidateCount, acceptedCount,
        static_cast<unsigned>(decoded.isLast));
    if (candidateCount == 0 && decoded.isLast) {
        Log("voice",
            "Player completed %s ICE gathering without a candidate; the advertised TURN service is unavailable or unreachable",
            decoded.controlPath == 0 ? "publishing" : "subscription");
    }
}

static void RtcPublishingStateChanged719(
    int peer, rtcState state, void* userData) {
    auto* session = static_cast<VoicePublishingPeer719*>(userData);
    Log("voice", "WebRTC publishing peer=%d state=%d", peer,
        static_cast<int>(state));
    if (!session || (state != RTC_FAILED && state != RTC_CLOSED))
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->failed = true;
    }
    session->stateChanged.notify_all();
}

static void RtcPublishingIceStateChanged719(
    int peer, rtcIceState state, void* userData) {
    auto* session = static_cast<VoicePublishingPeer719*>(userData);
    Log("voice", "WebRTC publishing peer=%d ICE state=%d", peer,
        static_cast<int>(state));
    if (!session || state != RTC_ICE_FAILED)
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->failed = true;
    }
    session->stateChanged.notify_all();
}

static void RtcPublishingGatheringChanged719(
    int peer, rtcGatheringState state, void* userData) {
    auto* session = static_cast<VoicePublishingPeer719*>(userData);
    Log("voice", "WebRTC publishing peer=%d gathering state=%d", peer,
        static_cast<int>(state));
    if (!session || state != RTC_GATHERING_COMPLETE)
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->gatheringComplete = true;
    }
    session->stateChanged.notify_all();
}

static void RtcPublishingCandidate719(
    int peer, const char* candidate, const char* mid, void*) {
    Log("voice",
        "WebRTC publishing peer=%d gathered candidate mid=%s bytes=%zu",
        peer, mid ? mid : "<none>", candidate ? std::strlen(candidate) : 0);
}

static bool RewriteRtpMid719(
    std::vector<char>& packet, int extensionId, const std::string& mid) {
    if (packet.size() < 16 || extensionId <= 0 || extensionId >= 15 ||
        mid.empty() || mid.size() > 16) {
        return false;
    }

    const auto* bytes = reinterpret_cast<const uint8_t*>(packet.data());
    if ((bytes[0] >> 6) != 2 || (bytes[0] & 0x10) == 0 ||
        (bytes[1] >= 192 && bytes[1] <= 223)) {
        return false;
    }

    const size_t extensionHeader = 12 + 4 * (bytes[0] & 0x0f);
    if (extensionHeader + 4 > packet.size())
        return false;
    const uint16_t profile = static_cast<uint16_t>(
        (static_cast<uint16_t>(bytes[extensionHeader]) << 8) |
        bytes[extensionHeader + 1]);
    if (profile != 0xbede)
        return false;
    const size_t extensionBytes = 4 * static_cast<size_t>(
        (static_cast<uint16_t>(bytes[extensionHeader + 2]) << 8) |
        bytes[extensionHeader + 3]);
    const size_t extensionEnd = extensionHeader + 4 + extensionBytes;
    if (extensionEnd > packet.size())
        return false;

    size_t cursor = extensionHeader + 4;
    while (cursor < extensionEnd) {
        const uint8_t header = static_cast<uint8_t>(packet[cursor++]);
        if (header == 0)
            continue;
        const int id = header >> 4;
        if (id == 15)
            break;
        const size_t length = (header & 0x0f) + 1;
        if (cursor + length > extensionEnd)
            return false;
        if (id == extensionId) {
            if (length != mid.size())
                return false;
            std::memcpy(packet.data() + cursor, mid.data(), length);
            return true;
        }
        cursor += length;
    }
    return false;
}

static void RtcPublishingTrackMessage719(
    int track, const char* message, int size, void* userData) {
    auto* session = static_cast<VoicePublishingPeer719*>(userData);
    if (!session || !message || size <= 0)
        return;

    const uint64_t packet = session->receivedPackets.fetch_add(1) + 1;
    if (session->publishingCompletionState.load() == 0) {
        uint8_t expected = 0;
        if (session->publishingCompletionState.compare_exchange_strong(
                expected, 1)) {
            const bool sent = SendPublishingHandshakeCompleted719(
                session->voiceChatService, session->senderContext,
                session->sessionId);
            session->publishingCompletionState.store(sent ? 2 : 0);
        }
    }
    bool hasRemotePublisher = false;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (const VoicePublishingPeer719* publisher :
             gVoicePublishingPeers719) {
            if (publisher && publisher->senderContext != session->senderContext) {
                hasRemotePublisher = true;
                break;
            }
        }
    }
    // A one-participant server still needs the independent initial subscription
    // operation completed. Do not send the empty marker once another publisher
    // exists, because a real SubscriptionHandshakeInitiated follows instead.
    if (session->publishingCompletionState.load() == 2 &&
        !hasRemotePublisher &&
        !session->subscriptionInitialBatchEmptySent.load()) {
        bool expected = false;
        if (session->subscriptionInitialBatchEmptySent.compare_exchange_strong(
                expected, true) &&
            !SendVoiceSubscriptionInitialBatchEmpty719(
                session->voiceChatService, session->senderContext,
                session->sessionId)) {
            session->subscriptionInitialBatchEmptySent.store(false);
        }
    }

    uint64_t forwarded = 0;
    uint64_t dropped = 0;
    {
        // Active subscription objects and tracks are process-lifetime stable.
        // Holding the registry lock also prevents a peer from being retired
        // while rtcSendMessage queues this packet.
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (VoiceSubscriptionPeer719* subscriber :
             gVoiceSubscriptionPeers719) {
            if (!subscriber || !subscriber->answerApplied.load())
                continue;
            for (const auto& routeOwner : subscriber->routes) {
                VoiceSubscriptionRoute719* route = routeOwner.get();
                if (!route ||
                    route->sourceSenderContext != session->senderContext ||
                    route->track < 0) {
                    continue;
                }

                const char* outgoing = message;
                std::vector<char> rewritten;
                if (route->mid != route->sourceMid &&
                    route->midExtensionId > 0) {
                    rewritten.assign(message, message + size);
                    if (RewriteRtpMid719(
                            rewritten, route->midExtensionId, route->mid)) {
                        outgoing = rewritten.data();
                    }
                }

                const int result = rtcIsOpen(route->track)
                    ? rtcSendMessage(route->track, outgoing, size)
                    : RTC_ERR_NOT_AVAIL;
                if (result >= 0) {
                    ++forwarded;
                    route->forwardedPackets.fetch_add(1);
                } else {
                    ++dropped;
                    const uint64_t routeDropped =
                        route->droppedPackets.fetch_add(1) + 1;
                    if (routeDropped <= 5 || routeDropped % 500 == 0) {
                        Log("voice",
                            "WebRTC subscription track=%d could not forward sourceSession=%s targetSession=%s result=%d dropped=%llu open=%u",
                            route->track, session->sessionId.c_str(),
                            subscriber->sessionId.c_str(), result,
                            static_cast<unsigned long long>(routeDropped),
                            static_cast<unsigned>(rtcIsOpen(route->track)));
                    }
                }
            }
        }
    }
    if (packet <= 5 || packet % 5000 == 0) {
        Log("voice",
            "WebRTC publishing track=%d received media packet=%llu size=%d session=%s fanout=%llu dropped=%llu",
            track, static_cast<unsigned long long>(packet), size,
            session->sessionId.c_str(),
            static_cast<unsigned long long>(forwarded),
            static_cast<unsigned long long>(dropped));
    }
}

static void RtcPublishingTrack719(int peer, int track, void* userData) {
    auto* session = static_cast<VoicePublishingPeer719*>(userData);
    Log("voice", "WebRTC publishing peer=%d accepted remote audio track=%d",
        peer, track);
    if (!session)
        return;

    session->incomingTrack.store(track);
    rtcSetUserPointer(track, session);
    const int callbackResult = rtcSetMessageCallback(
        track, &RtcPublishingTrackMessage719);
    if (callbackResult < 0) {
        Log("voice",
            "WebRTC publishing track=%d message callback failed result=%d",
            track, callbackResult);
    }
}

static void RetireVoicePublishingPeer719(VoicePublishingPeer719* session) {
    if (!session)
        return;

    const int incomingTrack = session->incomingTrack.exchange(-1);
    if (incomingTrack >= 0) {
        rtcSetMessageCallback(incomingTrack, nullptr);
        rtcSetUserPointer(incomingTrack, nullptr);
    }
    if (session->peer >= 0) {
        const int peer = session->peer;
        rtcSetTrackCallback(peer, nullptr);
        rtcSetLocalCandidateCallback(peer, nullptr);
        rtcSetGatheringStateChangeCallback(peer, nullptr);
        rtcSetIceStateChangeCallback(peer, nullptr);
        rtcSetStateChangeCallback(peer, nullptr);
        rtcSetUserPointer(peer, nullptr);
        rtcClosePeerConnection(peer);
        rtcDeletePeerConnection(peer);
        session->peer = -1;
        Log("voice", "Destroyed WebRTC publishing peer=%d session=%s", peer,
            session->sessionId.c_str());
    }
}

static void DestroyVoicePublishingPeer719(
    std::unique_ptr<VoicePublishingPeer719>& session) {
    RetireVoicePublishingPeer719(session.get());
    session.reset();
}

static void RtcSubscriptionStateChanged719(
    int peer, rtcState state, void* userData) {
    auto* session = static_cast<VoiceSubscriptionPeer719*>(userData);
    Log("voice", "WebRTC subscription peer=%d state=%d session=%s", peer,
        static_cast<int>(state), session ? session->sessionId.c_str() : "<none>");
    if (!session)
        return;
    if (state == RTC_CONNECTED)
        session->connected.store(true);
    if (state != RTC_FAILED && state != RTC_CLOSED)
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->failed = true;
    }
    session->stateChanged.notify_all();
}

static void RtcSubscriptionIceStateChanged719(
    int peer, rtcIceState state, void* userData) {
    auto* session = static_cast<VoiceSubscriptionPeer719*>(userData);
    Log("voice", "WebRTC subscription peer=%d ICE state=%d session=%s", peer,
        static_cast<int>(state), session ? session->sessionId.c_str() : "<none>");
    if (!session || state != RTC_ICE_FAILED)
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->failed = true;
    }
    session->stateChanged.notify_all();
}

static void RtcSubscriptionGatheringChanged719(
    int peer, rtcGatheringState state, void* userData) {
    auto* session = static_cast<VoiceSubscriptionPeer719*>(userData);
    Log("voice",
        "WebRTC subscription peer=%d gathering state=%d session=%s", peer,
        static_cast<int>(state), session ? session->sessionId.c_str() : "<none>");
    if (!session || state != RTC_GATHERING_COMPLETE)
        return;

    {
        std::lock_guard<std::mutex> lock(session->stateMutex);
        session->gatheringComplete = true;
    }
    session->stateChanged.notify_all();
}

static void RtcSubscriptionCandidate719(
    int peer, const char* candidate, const char* mid, void* userData) {
    auto* session = static_cast<VoiceSubscriptionPeer719*>(userData);
    Log("voice",
        "WebRTC subscription peer=%d gathered candidate mid=%s bytes=%zu session=%s",
        peer, mid ? mid : "<none>",
        candidate ? std::strlen(candidate) : 0,
        session ? session->sessionId.c_str() : "<none>");
}

static void RtcSubscriptionTrackOpened719(int track, void* userData) {
    auto* route = static_cast<VoiceSubscriptionRoute719*>(userData);
    Log("voice",
        "WebRTC subscription outbound track=%d opened targetSession=%s sourceUserId=%lld ssrc=%u",
        track, route ? route->targetSessionId.c_str() : "<none>",
        route ? static_cast<long long>(route->userId) : 0LL,
        route ? route->ssrc : 0);
}

static void RtcSubscriptionTrackMessage719(
    int track, const char* message, int size, void* userData) {
    auto* route = static_cast<VoiceSubscriptionRoute719*>(userData);
    if (!route || !message || size < 2)
        return;

    const uint8_t packetType = static_cast<uint8_t>(message[1]);
    if (packetType < 192 || packetType > 223) {
        Log("voice",
            "Ignored non-RTCP packet received on send-only subscription track=%d size=%d packetType=%u",
            track, size, static_cast<unsigned>(packetType));
        return;
    }

    int publishingTrack = -1;
    int result = RTC_ERR_NOT_AVAIL;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (VoicePublishingPeer719* publisher : gVoicePublishingPeers719) {
            if (!publisher || publisher->senderContext !=
                    route->sourceSenderContext) {
                continue;
            }
            publishingTrack = publisher->incomingTrack.load();
            if (publishingTrack >= 0 && rtcIsOpen(publishingTrack))
                result = rtcSendMessage(publishingTrack, message, size);
            break;
        }
    }

    const uint64_t packet = route->feedbackPackets.fetch_add(1) + 1;
    if (packet <= 5 || packet % 5000 == 0 || result < 0) {
        Log("voice",
            "Forwarded subscription RTCP feedback packet=%llu size=%d subscriptionTrack=%d publishingTrack=%d result=%d targetSession=%s",
            static_cast<unsigned long long>(packet), size, track,
            publishingTrack, result, route->targetSessionId.c_str());
    }
}

static void RetireVoiceSubscriptionPeer719(VoiceSubscriptionPeer719* session) {
    if (!session)
        return;

    for (const auto& route : session->routes) {
        if (!route || route->track < 0)
            continue;
        rtcSetOpenCallback(route->track, nullptr);
        rtcSetMessageCallback(route->track, nullptr);
        rtcSetUserPointer(route->track, nullptr);
        rtcDeleteTrack(route->track);
        route->track = -1;
    }
    if (session->peer >= 0) {
        const int peer = session->peer;
        rtcSetLocalCandidateCallback(peer, nullptr);
        rtcSetGatheringStateChangeCallback(peer, nullptr);
        rtcSetIceStateChangeCallback(peer, nullptr);
        rtcSetStateChangeCallback(peer, nullptr);
        rtcSetUserPointer(peer, nullptr);
        rtcClosePeerConnection(peer);
        rtcDeletePeerConnection(peer);
        session->peer = -1;
        Log("voice", "Destroyed WebRTC subscription peer=%d session=%s", peer,
            session->sessionId.c_str());
    }
}

static void DestroyVoiceSubscriptionPeer719(
    std::unique_ptr<VoiceSubscriptionPeer719>& session) {
    RetireVoiceSubscriptionPeer719(session.get());
    session.reset();
}

static bool IsVoicePublishingPeerFailed719(VoicePublishingPeer719* session) {
    if (!session || session->peer < 0)
        return true;
    std::lock_guard<std::mutex> lock(session->stateMutex);
    return session->failed;
}

static bool IsVoiceSubscriptionPeerFailed719(VoiceSubscriptionPeer719* session) {
    if (!session || session->peer < 0)
        return true;
    std::lock_guard<std::mutex> lock(session->stateMutex);
    return session->failed;
}

static void PruneFailedVoicePeers719() {
    std::vector<VoicePublishingPeer719*> retiredPublishers;
    std::vector<VoiceSubscriptionPeer719*> retiredSubscriptions;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (auto iterator = gVoicePublishingPeers719.begin();
             iterator != gVoicePublishingPeers719.end();) {
            if (!IsVoicePublishingPeerFailed719(*iterator)) {
                ++iterator;
                continue;
            }
            if (*iterator)
                retiredPublishers.push_back(*iterator);
            iterator = gVoicePublishingPeers719.erase(iterator);
        }

        // Removing a publishing transport invalidates source tracks in every subscription. Rebuild
        // those transports from the remaining publishers instead of retaining silent/stale routes.
        if (!retiredPublishers.empty()) {
            for (VoiceSubscriptionPeer719* subscription :
                 gVoiceSubscriptionPeers719) {
                if (subscription)
                    retiredSubscriptions.push_back(subscription);
            }
            gVoiceSubscriptionPeers719.clear();
        } else {
            for (auto iterator = gVoiceSubscriptionPeers719.begin();
                 iterator != gVoiceSubscriptionPeers719.end();) {
                if (!IsVoiceSubscriptionPeerFailed719(*iterator)) {
                    ++iterator;
                    continue;
                }
                if (*iterator)
                    retiredSubscriptions.push_back(*iterator);
                iterator = gVoiceSubscriptionPeers719.erase(iterator);
            }
        }

        gRetiredVoicePublishingPeers719.insert(
            gRetiredVoicePublishingPeers719.end(),
            retiredPublishers.begin(), retiredPublishers.end());
        gRetiredVoiceSubscriptionPeers719.insert(
            gRetiredVoiceSubscriptionPeers719.end(),
            retiredSubscriptions.begin(), retiredSubscriptions.end());
    }

    for (VoicePublishingPeer719* publisher : retiredPublishers)
        RetireVoicePublishingPeer719(publisher);
    for (VoiceSubscriptionPeer719* subscription : retiredSubscriptions)
        RetireVoiceSubscriptionPeer719(subscription);

    if (!retiredPublishers.empty() || !retiredSubscriptions.empty()) {
        Log("voice", "Pruned failed voice peers publishers=%zu subscriptions=%zu",
            retiredPublishers.size(), retiredSubscriptions.size());
    }
}

static bool BuildSubscriptionTrackDescription719(
    const VoicePublishingPeer719& source, const std::string& targetMid,
    std::string& mediaDescription, std::string& sourceMid,
    int& midExtensionId) {
    const size_t audioStart = source.decodedOffer.find("m=audio ");
    if (audioStart == std::string::npos)
        return false;
    const size_t audioEnd = source.decodedOffer.find("\nm=", audioStart + 1);
    const size_t sectionEnd = audioEnd == std::string::npos
        ? source.decodedOffer.size()
        : audioEnd + 1;

    mediaDescription.clear();
    sourceMid = "0";
    midExtensionId = -1;
    bool wroteMediaLine = false;
    size_t cursor = audioStart;
    while (cursor < sectionEnd) {
        size_t lineEnd = source.decodedOffer.find('\n', cursor);
        if (lineEnd == std::string::npos || lineEnd > sectionEnd)
            lineEnd = sectionEnd;
        std::string line = source.decodedOffer.substr(cursor, lineEnd - cursor);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (!wroteMediaLine && line.starts_with("m=audio ")) {
            mediaDescription.append(line.substr(2));
            mediaDescription.append("\r\n");
            wroteMediaLine = true;
        } else if (line.starts_with("a=mid:")) {
            sourceMid = line.substr(std::strlen("a=mid:"));
        } else {
            const bool copyLine =
                line == "a=rtcp-mux" || line == "a=rtcp-rsize" ||
                line == "a=extmap-allow-mixed" ||
                line.starts_with("a=extmap:") ||
                line.starts_with("a=rtpmap:") ||
                line.starts_with("a=fmtp:") ||
                line.starts_with("a=rtcp-fb:") ||
                line.starts_with("a=ssrc:") ||
                line.starts_with("a=ssrc-group:") ||
                line.starts_with("a=msid:") ||
                line.starts_with("a=ptime:") ||
                line.starts_with("a=maxptime:");
            if (copyLine) {
                mediaDescription.append(line);
                mediaDescription.append("\r\n");
            }

            if (line.starts_with("a=extmap:") &&
                line.find("urn:ietf:params:rtp-hdrext:sdes:mid") !=
                    std::string::npos) {
                size_t number = std::strlen("a=extmap:");
                int value = 0;
                while (number < line.size() && line[number] >= '0' &&
                       line[number] <= '9') {
                    value = value * 10 + (line[number] - '0');
                    ++number;
                }
                if (value > 0 && value < 15)
                    midExtensionId = value;
            }
        }

        if (lineEnd >= sectionEnd)
            break;
        cursor = lineEnd + 1;
    }

    if (!wroteMediaLine)
        return false;
    mediaDescription.append("a=mid:");
    mediaDescription.append(targetMid);
    mediaDescription.append("\r\na=sendonly\r\n");
    return true;
}

static std::unique_ptr<VoiceSubscriptionPeer719>
CreateVoiceSubscriptionPeer719(
    VoicePublishingPeer719* target,
    const std::vector<VoicePublishingPeer719*>& sources,
    std::string& offer, std::string& subscriptionState) {
    if (!target || sources.empty())
        return nullptr;

    rtcConfiguration configuration {};
    configuration.disableAutoNegotiation = true;
    configuration.forceMediaTransport = true;
    auto session = std::make_unique<VoiceSubscriptionPeer719>();
    session->sessionId = target->sessionId;
    session->voiceChatService = target->voiceChatService;
    session->senderContext = target->senderContext;
    session->eventTag = gNextVoiceSubscriptionEventTag719.fetch_add(1);
    session->peer = rtcCreatePeerConnection(&configuration);
    if (session->peer < 0) {
        Log("voice", "libdatachannel could not create subscription peer result=%d",
            session->peer);
        return nullptr;
    }

    const int peer = session->peer;
    rtcSetUserPointer(peer, session.get());
    const bool callbacksReady =
        rtcSetLocalCandidateCallback(
            peer, &RtcSubscriptionCandidate719) >= 0 &&
        rtcSetStateChangeCallback(
            peer, &RtcSubscriptionStateChanged719) >= 0 &&
        rtcSetIceStateChangeCallback(
            peer, &RtcSubscriptionIceStateChanged719) >= 0 &&
        rtcSetGatheringStateChangeCallback(
            peer, &RtcSubscriptionGatheringChanged719) >= 0;
    if (!callbacksReady) {
        Log("voice", "libdatachannel could not register subscription callbacks");
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }

    nlohmann::json states = nlohmann::json::array();
    for (VoicePublishingPeer719* source : sources) {
        if (!source || source->ssrc == 0 || session->routes.size() >= 10)
            continue;

        auto route = std::make_unique<VoiceSubscriptionRoute719>();
        route->sourceSenderContext = source->senderContext;
        route->userId = source->userId;
        route->feedId = source->feedId;
        route->ssrc = source->ssrc;
        route->mid = std::to_string(session->routes.size());
        route->targetSessionId = session->sessionId;

        std::string mediaDescription;
        if (!BuildSubscriptionTrackDescription719(
                *source, route->mid, mediaDescription, route->sourceMid,
                route->midExtensionId)) {
            Log("voice",
                "Could not derive subscription media description from source session=%s",
                source->sessionId.c_str());
            continue;
        }
        route->track = rtcAddTrack(peer, mediaDescription.c_str());
        if (route->track < 0) {
            Log("voice",
                "libdatachannel rejected subscription track sourceSession=%s mid=%s result=%d",
                source->sessionId.c_str(), route->mid.c_str(), route->track);
            continue;
        }
        rtcSetUserPointer(route->track, route.get());
        rtcSetOpenCallback(route->track, &RtcSubscriptionTrackOpened719);
        rtcSetMessageCallback(
            route->track, &RtcSubscriptionTrackMessage719);

        nlohmann::json state;
        state["userId"] = route->userId;
        state["feed"]["feedId"] = route->feedId;
        // Subscription mute is local to the receiving Player. Publisher
        // microphone state is reported separately by PublishStateChange and
        // must not seed the recipient's subscription mutelist. The initial
        // SubscriptionHandshakeAcked response can request muted users; the
        // current local control plane receives an empty usersToMute list.
        state["feed"]["isMuted"] = false;
        state["feed"]["tracks"] = nlohmann::json::array();
        nlohmann::json trackState;
        trackState["mid"] = route->mid;
        trackState["mediaType"] = "audio";
        state["feed"]["tracks"].push_back(std::move(trackState));
        states.push_back(std::move(state));

        Log("voice",
            "Added subscription route targetSession=%s sourceSession=%s userId=%lld feedId=%lld ssrc=%u sourceMid=%s targetMid=%s midExt=%d track=%d",
            target->sessionId.c_str(), source->sessionId.c_str(),
            static_cast<long long>(route->userId),
            static_cast<long long>(route->feedId), route->ssrc,
            route->sourceMid.c_str(), route->mid.c_str(),
            route->midExtensionId, route->track);
        session->routes.push_back(std::move(route));
    }

    if (session->routes.empty()) {
        Log("voice", "No usable publisher tracks for subscription session=%s",
            target->sessionId.c_str());
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }

    const int offerResult = rtcSetLocalDescription(peer, "offer");
    if (offerResult < 0) {
        Log("voice",
            "libdatachannel could not create subscription offer peer=%d result=%d",
            peer, offerResult);
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }
    {
        std::unique_lock<std::mutex> lock(session->stateMutex);
        const bool finished = session->stateChanged.wait_for(
            lock, std::chrono::seconds(8), [&session] {
                return session->gatheringComplete || session->failed;
            });
        if (!finished || !session->gatheringComplete || session->failed) {
            Log("voice",
                "WebRTC subscription offer gathering did not complete peer=%d timeout=%d failed=%d",
                peer, finished ? 0 : 1, session->failed ? 1 : 0);
            lock.unlock();
            DestroyVoiceSubscriptionPeer719(session);
            return nullptr;
        }
    }

    const int offerSize = rtcGetLocalDescription(peer, nullptr, 0);
    if (offerSize <= 1) {
        Log("voice",
            "libdatachannel returned no local subscription offer peer=%d result=%d",
            peer, offerSize);
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }
    std::vector<char> offerBuffer(static_cast<size_t>(offerSize));
    const int copiedSize = rtcGetLocalDescription(
        peer, offerBuffer.data(), static_cast<int>(offerBuffer.size()));
    if (copiedSize <= 1) {
        Log("voice",
            "libdatachannel could not copy local subscription offer peer=%d result=%d",
            peer, copiedSize);
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }

    offer.assign(offerBuffer.data(), static_cast<size_t>(copiedSize - 1));
    subscriptionState = states.dump();
    if (offer.find("a=candidate:") == std::string::npos) {
        Log("voice",
            "libdatachannel subscription offer has no ICE candidate; Player cannot establish media transport");
        DestroyVoiceSubscriptionPeer719(session);
        return nullptr;
    }
    return session;
}

static bool RenegotiateVoiceSubscription719(
    VoiceSubscriptionPeer719* session,
    const std::vector<VoicePublishingPeer719*>& sources) {
    if (!session || session->peer < 0 || sources.empty() ||
        session->negotiationPending.load()) {
        return false;
    }

    nlohmann::json states = nlohmann::json::array();
    size_t addedRoutes = 0;
    for (VoicePublishingPeer719* source : sources) {
        if (!source || source->ssrc == 0)
            continue;

        bool alreadyRouted = false;
        size_t routeIndex = 0;
        {
            std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
            routeIndex = session->routes.size();
            if (routeIndex >= 10)
                continue;
            for (const auto& existing : session->routes) {
                if (existing && existing->sourceSenderContext ==
                        source->senderContext) {
                    alreadyRouted = true;
                    break;
                }
            }
        }
        if (alreadyRouted)
            continue;

        auto route = std::make_unique<VoiceSubscriptionRoute719>();
        route->sourceSenderContext = source->senderContext;
        route->userId = source->userId;
        route->feedId = source->feedId;
        route->ssrc = source->ssrc;
        route->mid = std::to_string(routeIndex);
        route->targetSessionId = session->sessionId;

        std::string mediaDescription;
        if (!BuildSubscriptionTrackDescription719(
                *source, route->mid, mediaDescription, route->sourceMid,
                route->midExtensionId)) {
            Log("voice",
                "Could not derive renegotiated subscription track sourceSession=%s",
                source->sessionId.c_str());
            continue;
        }
        route->track = rtcAddTrack(session->peer, mediaDescription.c_str());
        if (route->track < 0) {
            Log("voice",
                "libdatachannel rejected renegotiated subscription track sourceSession=%s mid=%s result=%d",
                source->sessionId.c_str(), route->mid.c_str(), route->track);
            continue;
        }
        rtcSetUserPointer(route->track, route.get());
        rtcSetOpenCallback(route->track, &RtcSubscriptionTrackOpened719);
        rtcSetMessageCallback(
            route->track, &RtcSubscriptionTrackMessage719);

        nlohmann::json state;
        state["userId"] = route->userId;
        state["feed"]["feedId"] = route->feedId;
        state["feed"]["isMuted"] = false;
        state["feed"]["tracks"] = nlohmann::json::array();
        nlohmann::json trackState;
        trackState["mid"] = route->mid;
        trackState["mediaType"] = "audio";
        state["feed"]["tracks"].push_back(std::move(trackState));
        states.push_back(std::move(state));

        Log("voice",
            "Added renegotiated subscription route targetSession=%s sourceSession=%s userId=%lld feedId=%lld ssrc=%u sourceMid=%s targetMid=%s midExt=%d track=%d",
            session->sessionId.c_str(), source->sessionId.c_str(),
            static_cast<long long>(route->userId),
            static_cast<long long>(route->feedId), route->ssrc,
            route->sourceMid.c_str(), route->mid.c_str(),
            route->midExtensionId, route->track);
        {
            std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
            session->routes.push_back(std::move(route));
        }
        ++addedRoutes;
    }

    if (addedRoutes == 0)
        return false;

    const int offerResult = rtcSetLocalDescription(session->peer, "offer");
    if (offerResult < 0) {
        Log("voice",
            "libdatachannel could not create renegotiated subscription offer peer=%d result=%d",
            session->peer, offerResult);
        return false;
    }
    const int offerSize = rtcGetLocalDescription(session->peer, nullptr, 0);
    if (offerSize <= 1) {
        Log("voice",
            "libdatachannel returned no renegotiated subscription offer peer=%d result=%d",
            session->peer, offerSize);
        return false;
    }
    std::vector<char> offerBuffer(static_cast<size_t>(offerSize));
    const int copiedSize = rtcGetLocalDescription(
        session->peer, offerBuffer.data(), static_cast<int>(offerBuffer.size()));
    if (copiedSize <= 1) {
        Log("voice",
            "libdatachannel could not copy renegotiated subscription offer peer=%d result=%d",
            session->peer, copiedSize);
        return false;
    }

    const std::string offer(
        offerBuffer.data(), static_cast<size_t>(copiedSize - 1));
    const std::string state = states.dump();
    session->eventTag = gNextVoiceSubscriptionEventTag719.fetch_add(1);
    session->completionSent.store(false);
    session->negotiationPending.store(true);
    if (!SendSubscriptionHandshakeInitiated719(
            session->voiceChatService, session->senderContext, offer, state,
            false, session->sessionId, session->eventTag)) {
        session->negotiationPending.store(false);
        return false;
    }
    return true;
}

static void EnsureVoiceSubscriptions719() {
    PruneFailedVoicePeers719();

    std::vector<VoicePublishingPeer719*> publishers;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        publishers = gVoicePublishingPeers719;
    }

    for (VoicePublishingPeer719* target : publishers) {
        if (!target || !target->voiceChatService || !target->senderContext)
            continue;

        VoiceSubscriptionPeer719* existingSubscription = nullptr;
        std::vector<VoicePublishingPeer719*> missingSources;
        {
            std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
            for (VoiceSubscriptionPeer719* active :
                 gVoiceSubscriptionPeers719) {
                if (active && active->senderContext == target->senderContext) {
                    existingSubscription = active;
                    break;
                }
            }
            if (existingSubscription) {
                for (VoicePublishingPeer719* source : publishers) {
                    if (!source || source == target)
                        continue;
                    bool routed = false;
                    for (const auto& route : existingSubscription->routes) {
                        if (route && route->sourceSenderContext ==
                                source->senderContext) {
                            routed = true;
                            break;
                        }
                    }
                    if (!routed)
                        missingSources.push_back(source);
                }
            }
        }
        if (existingSubscription) {
            if (!missingSources.empty()) {
                if (existingSubscription->negotiationPending.load()) {
                    Log("voice",
                        "Subscription session=%s deferred %zu new source(s) while eventTag=%lld is pending",
                        target->sessionId.c_str(), missingSources.size(),
                        static_cast<long long>(
                            existingSubscription->eventTag));
                } else if (!RenegotiateVoiceSubscription719(
                               existingSubscription, missingSources)) {
                    Log("voice",
                        "Subscription session=%s could not renegotiate %zu new source(s)",
                        target->sessionId.c_str(), missingSources.size());
                }
            } else {
                Log("voice",
                    "Subscription session=%s already routes every active remote publisher",
                    target->sessionId.c_str());
            }
            continue;
        }

        std::vector<VoicePublishingPeer719*> sources;
        for (VoicePublishingPeer719* source : publishers) {
            if (source && source != target)
                sources.push_back(source);
        }
        if (sources.empty())
            continue;

        std::string offer;
        std::string state;
        auto subscription = CreateVoiceSubscriptionPeer719(
            target, sources, offer, state);
        if (!subscription)
            continue;

        VoiceSubscriptionPeer719* active = subscription.get();
        bool registered = false;
        {
            std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
            bool raced = false;
            for (const VoiceSubscriptionPeer719* candidate :
                 gVoiceSubscriptionPeers719) {
                if (candidate &&
                    candidate->senderContext == active->senderContext) {
                    raced = true;
                    break;
                }
            }
            if (!raced) {
                gVoiceSubscriptionPeers719.push_back(active);
                subscription.release();
                registered = true;
            }
        }
        if (!registered) {
            DestroyVoiceSubscriptionPeer719(subscription);
            continue;
        }

        active->negotiationPending.store(true);
        if (!SendSubscriptionHandshakeInitiated719(
                active->voiceChatService, active->senderContext, offer, state,
                true, active->sessionId, active->eventTag)) {
            active->negotiationPending.store(false);
            std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
            for (auto iterator = gVoiceSubscriptionPeers719.begin();
                 iterator != gVoiceSubscriptionPeers719.end(); ++iterator) {
                if (*iterator == active) {
                    gVoiceSubscriptionPeers719.erase(iterator);
                    break;
                }
            }
            gRetiredVoiceSubscriptionPeers719.push_back(active);
            rtcClosePeerConnection(active->peer);
        }
    }
}

struct VoicePublisherMuteSnapshot719 {
    int64_t userId = 0;
    int muteState = 1;
    std::string sessionId;
};

static void ReplaySubscriptionMuteStates719(
    uint8_t* voiceChatService, uint64_t recipientSenderContext,
    VoiceSubscriptionPeer719* subscription) {
    std::vector<VoicePublisherMuteSnapshot719> snapshots;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (const auto& route : subscription->routes) {
            if (!route)
                continue;
            for (VoicePublishingPeer719* publisher :
                 gVoicePublishingPeers719) {
                if (!publisher || publisher->senderContext !=
                        route->sourceSenderContext) {
                    continue;
                }
                snapshots.push_back({
                    publisher->userId,
                    publisher->muteState.load(),
                    publisher->sessionId,
                });
                break;
            }
        }
    }

    for (const VoicePublisherMuteSnapshot719& snapshot : snapshots) {
        SendVoicePlayerMuteStatusChanged719(
            voiceChatService, recipientSenderContext, snapshot.userId,
            snapshot.muteState, snapshot.sessionId);
    }
    Log("voice",
        "Replayed %zu publisher mute state(s) after subscription completion recipient=0x%llx session=%s",
        snapshots.size(),
        static_cast<unsigned long long>(recipientSenderContext),
        subscription->sessionId.c_str());
}

static void HandleSubscriptionHandshakeAcked719(
    uint8_t* voiceChatService, void* rawArguments,
    uint64_t senderContext) {
    SubscriptionHandshakeAcked719 decoded;
    if (!DecodeSubscriptionHandshakeAcked719(rawArguments, decoded))
        return;

    VoiceSubscriptionPeer719* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (VoiceSubscriptionPeer719* candidate :
             gVoiceSubscriptionPeers719) {
            if (candidate && candidate->senderContext == senderContext &&
                (decoded.sessionId.empty() ||
                 candidate->sessionId == decoded.sessionId) &&
                candidate->eventTag == decoded.eventTag) {
                session = candidate;
                break;
            }
        }
    }
    if (!session || session->peer < 0) {
        Log("voice",
            "SubscriptionHandshakeAcked has no matching peer senderContext=0x%llx session=%s eventTag=%lld",
            static_cast<unsigned long long>(senderContext),
            decoded.sessionId.c_str(),
            static_cast<long long>(decoded.eventTag));
        return;
    }
    if (decoded.answer.empty()) {
        Log("voice",
            "SubscriptionHandshakeAcked returned an empty SDP answer peer=%d",
            session->peer);
        return;
    }

    const int result = rtcSetRemoteDescription(
        session->peer, decoded.answer.c_str(), "answer");
    if (result < 0) {
        session->negotiationPending.store(false);
        Log("voice",
            "libdatachannel rejected Player subscription answer peer=%d session=%s eventTag=%lld result=%d",
            session->peer, session->sessionId.c_str(),
            static_cast<long long>(session->eventTag), result);
        return;
    }
    session->answerApplied.store(true);
    session->negotiationPending.store(false);
    Log("voice",
        "Applied Player subscription answer peer=%d session=%s eventTag=%lld usersToMute=%s",
        session->peer, session->sessionId.c_str(),
        static_cast<long long>(session->eventTag),
        decoded.usersToMute.empty() ? "<empty>" : decoded.usersToMute.c_str());

    bool completionSent = false;
    if (!session->completionSent.exchange(true)) {
        completionSent = SendSubscriptionHandshakeCompleted719(
            voiceChatService, senderContext, session->sessionId,
            session->eventTag);
        if (!completionSent)
            session->completionSent.store(false);
    }
    // The receiving client only has a remote voice participant after it has
    // accepted the subscription. Initial mute events sent when the publisher
    // registered arrived before this point and were discarded. Replay the
    // current state in the same reliable event stream, immediately after the
    // completion event that makes the participant usable.
    if (completionSent)
        ReplaySubscriptionMuteStates719(
            voiceChatService, senderContext, session);
    EnsureVoiceSubscriptions719();
}

static void HandlePublishStateChange719(
    uint8_t* voiceChatService, void* rawArguments, uint64_t senderContext) {
    int muteState = 0;
    std::string sessionId;
    if (!DecodePublishStateChange719(rawArguments, muteState, sessionId))
        return;

    bool updated = false;
    int64_t userId = 0;
    std::vector<uint64_t> recipients;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (VoicePublishingPeer719* publisher : gVoicePublishingPeers719) {
            if (publisher && publisher->senderContext == senderContext &&
                publisher->sessionId == sessionId) {
                publisher->muteState.store(muteState);
                Log("voice",
                    "Updated publisher mute state participantId=%lld session=%s muteState=%d",
                    static_cast<long long>(publisher->participantId),
                    publisher->sessionId.c_str(), muteState);
                userId = publisher->userId;
                updated = true;
                break;
            }
        }
        if (updated) {
            for (VoicePublishingPeer719* publisher : gVoicePublishingPeers719) {
                if (publisher && publisher->senderContext)
                    recipients.push_back(publisher->senderContext);
            }
        }
    }
    if (updated) {
        SendVoicePlayerMuteStateChanged719(
            voiceChatService, senderContext, muteState);
        // The one-argument VoiceChatPlayerMuteStateChanged event is a native
        // state relay. The client UI/voice model consumes this separate
        // game-data event, whose exact 0.719 contract is
        // (userId, isMuted, sessionId), after a publish operation succeeds.
        for (uint64_t recipient : recipients) {
            SendVoicePlayerMuteStatusChanged719(
                voiceChatService, recipient, userId, muteState, sessionId);
        }
        EnsureVoiceSubscriptions719();
    }
}

static bool AddPublishingSsrcToAnswer719(
    const std::string& offer, std::string& answer, uint32_t* ssrcOut) {
    const size_t offerAudio = offer.find("m=audio ");
    if (offerAudio == std::string::npos) {
        Log("voice", "Player publishing offer has no audio media section");
        return false;
    }

    const size_t offerAudioEnd = offer.find("\nm=", offerAudio + 1);
    constexpr const char* kSsrcPrefix = "\na=ssrc:";
    const size_t offerSsrc = offer.find(kSsrcPrefix, offerAudio);
    if (offerSsrc == std::string::npos ||
        (offerAudioEnd != std::string::npos && offerSsrc >= offerAudioEnd)) {
        Log("voice", "Player publishing offer audio section has no SSRC");
        return false;
    }

    const size_t ssrcStart = offerSsrc + std::strlen(kSsrcPrefix);
    const size_t ssrcEnd = offer.find_first_of(" \r\n", ssrcStart);
    if (ssrcEnd == std::string::npos || ssrcEnd == ssrcStart ||
        (offerAudioEnd != std::string::npos && ssrcEnd >= offerAudioEnd)) {
        Log("voice", "Player publishing offer has a malformed audio SSRC");
        return false;
    }

    uint64_t ssrcValue = 0;
    for (size_t index = ssrcStart; index < ssrcEnd; ++index) {
        const char character = offer[index];
        if (character < '0' || character > '9') {
            Log("voice", "Player publishing offer has a non-numeric audio SSRC");
            return false;
        }
        ssrcValue = ssrcValue * 10 + static_cast<uint64_t>(character - '0');
        if (ssrcValue > 0xFFFFFFFFULL) {
            Log("voice", "Player publishing offer audio SSRC exceeds uint32");
            return false;
        }
    }
    if (ssrcValue == 0) {
        Log("voice", "Player publishing offer has a zero audio SSRC");
        return false;
    }
    if (ssrcOut)
        *ssrcOut = static_cast<uint32_t>(ssrcValue);

    const size_t answerAudio = answer.find("m=audio ");
    if (answerAudio == std::string::npos) {
        Log("voice", "libdatachannel publishing answer has no audio media section");
        return false;
    }

    const size_t answerAudioEnd = answer.find("\nm=", answerAudio + 1);
    const size_t answerSsrc = answer.find(kSsrcPrefix, answerAudio);
    if (answerSsrc != std::string::npos &&
        (answerAudioEnd == std::string::npos || answerSsrc < answerAudioEnd)) {
        return true;
    }

    const std::string lineEnding =
        answer.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    const std::string ssrc = offer.substr(ssrcStart, ssrcEnd - ssrcStart);
    const std::string ssrcLine =
        "a=ssrc:" + ssrc + " cname:noobwarrior-localrcc" + lineEnding;

    size_t insertion = answer.find("a=end-of-candidates", answerAudio);
    if (insertion == std::string::npos ||
        (answerAudioEnd != std::string::npos && insertion >= answerAudioEnd)) {
        if (answerAudioEnd != std::string::npos) {
            insertion = answerAudioEnd + 1;
        } else {
            if (!answer.empty() && answer.back() != '\n')
                answer.append(lineEnding);
            insertion = answer.size();
        }
    }
    answer.insert(insertion, ssrcLine);
    Log("voice", "Echoed Player publishing SSRC %s into WebRTC answer",
        ssrc.c_str());
    return true;
}

static std::unique_ptr<VoicePublishingPeer719>
CreateVoicePublishingPeer719(
    const std::string& decodedOffer, const std::string& sessionId,
    std::string& plainAnswer) {
    rtcConfiguration configuration {};
    configuration.forceMediaTransport = true;
    auto session = std::make_unique<VoicePublishingPeer719>();
    session->sessionId = sessionId;
    session->decodedOffer = decodedOffer;
    session->peer = rtcCreatePeerConnection(&configuration);
    if (session->peer < 0) {
        Log("voice", "libdatachannel could not create publishing peer result=%d",
            session->peer);
        return nullptr;
    }

    const int peer = session->peer;
    rtcSetUserPointer(peer, session.get());
    const bool callbacksReady =
        rtcSetLocalCandidateCallback(
            peer, &RtcPublishingCandidate719) >= 0 &&
        rtcSetStateChangeCallback(
            peer, &RtcPublishingStateChanged719) >= 0 &&
        rtcSetIceStateChangeCallback(
            peer, &RtcPublishingIceStateChanged719) >= 0 &&
        rtcSetGatheringStateChangeCallback(
            peer, &RtcPublishingGatheringChanged719) >= 0 &&
        rtcSetTrackCallback(peer, &RtcPublishingTrack719) >= 0;
    if (!callbacksReady) {
        Log("voice", "libdatachannel could not register publishing peer callbacks");
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }

    const int remoteDescriptionResult = rtcSetRemoteDescription(
        peer, decodedOffer.c_str(), "offer");
    if (remoteDescriptionResult < 0) {
        Log("voice",
            "libdatachannel rejected Player publishing offer result=%d",
            remoteDescriptionResult);
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }

    {
        std::unique_lock<std::mutex> lock(session->stateMutex);
        const bool finished = session->stateChanged.wait_for(
            lock, std::chrono::seconds(8), [&session] {
                return session->gatheringComplete || session->failed;
            });
        if (!finished || !session->gatheringComplete || session->failed) {
            Log("voice",
                "WebRTC publishing answer gathering did not complete peer=%d timeout=%d failed=%d",
                peer, finished ? 0 : 1, session->failed ? 1 : 0);
            lock.unlock();
            DestroyVoicePublishingPeer719(session);
            return nullptr;
        }
    }

    const int answerSize = rtcGetLocalDescription(peer, nullptr, 0);
    if (answerSize <= 1) {
        Log("voice",
            "libdatachannel returned no local publishing answer peer=%d result=%d",
            peer, answerSize);
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }

    std::vector<char> answerBuffer(static_cast<size_t>(answerSize));
    const int copiedSize = rtcGetLocalDescription(
        peer, answerBuffer.data(), static_cast<int>(answerBuffer.size()));
    if (copiedSize <= 1) {
        Log("voice",
            "libdatachannel could not copy local publishing answer peer=%d result=%d",
            peer, copiedSize);
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }

    plainAnswer.assign(answerBuffer.data(), static_cast<size_t>(copiedSize - 1));
    if (!AddPublishingSsrcToAnswer719(
            decodedOffer, plainAnswer, &session->ssrc)) {
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }
    if (plainAnswer.find("a=candidate:") == std::string::npos) {
        Log("voice",
            "libdatachannel answer has no ICE candidate; Player cannot establish media transport");
        DestroyVoicePublishingPeer719(session);
        return nullptr;
    }
    return session;
}

static void HandlePublishingHandshakeInitiated719(
    uint8_t* voiceChatService, void* rawArguments, uint64_t senderContext) {
    std::string decodedOffer;
    std::string sessionId;
    int muteState = 0;
    if (!DecodePublishingHandshakeInitiated719(
            rawArguments, decodedOffer, sessionId, &muteState)) {
        return;
    }

    std::string plainAnswer;
    auto session = CreateVoicePublishingPeer719(
        decodedOffer, sessionId, plainAnswer);
    if (!session)
        return;

    session->voiceChatService = voiceChatService;
    session->senderContext = senderContext;
    const VoiceParticipantIdentity719 identity =
        GetOrCreateVoiceParticipantIdentity719(senderContext);
    session->participantId = identity.participantId;
    session->userId = identity.userId;
    session->feedId = 0x100000000LL + identity.participantId;
    session->muteState.store(muteState);

    std::string encodedAnswer;
    if (!EncodePublishingAnswer719(plainAnswer, encodedAnswer) ||
        !SendPublishingHandshakeAcked719(
            voiceChatService, senderContext, encodedAnswer, sessionId)) {
        DestroyVoicePublishingPeer719(session);
        return;
    }

    VoicePublishingPeer719* active = session.get();
    std::vector<VoicePublishingPeer719*> previousSessions;
    std::vector<VoiceSubscriptionPeer719*> replacedSubscriptions;
    {
        std::lock_guard<std::mutex> lock(gVoicePeerMutex719);
        for (auto iterator = gVoicePublishingPeers719.begin();
             iterator != gVoicePublishingPeers719.end();) {
            VoicePublishingPeer719* candidate = *iterator;
            const bool sameSender = candidate &&
                candidate->senderContext == senderContext;
            const bool sameUser = candidate && active->userId > 0 &&
                candidate->userId == active->userId;
            if (!sameSender && !sameUser) {
                ++iterator;
                continue;
            }
            previousSessions.push_back(candidate);
            iterator = gVoicePublishingPeers719.erase(iterator);
        }

        // A replacement needs fresh subscription tracks even when senderContext is unchanged. The
        // old code saw the matching context and incorrectly treated the dead route as already wired,
        // leaving the reconnected microphone inaudible. Rebuilding also removes duplicate tracks
        // left by full game rejoins where the same UserId arrived with a new senderContext.
        if (!previousSessions.empty()) {
            replacedSubscriptions = gVoiceSubscriptionPeers719;
            gVoiceSubscriptionPeers719.clear();
        }
        gVoicePublishingPeers719.push_back(active);
        session.release();
        gRetiredVoicePublishingPeers719.insert(
            gRetiredVoicePublishingPeers719.end(),
            previousSessions.begin(), previousSessions.end());
        gRetiredVoiceSubscriptionPeers719.insert(
            gRetiredVoiceSubscriptionPeers719.end(),
            replacedSubscriptions.begin(), replacedSubscriptions.end());
    }

    for (VoicePublishingPeer719* previousSession : previousSessions)
        RetireVoicePublishingPeer719(previousSession);
    for (VoiceSubscriptionPeer719* previousSubscription :
         replacedSubscriptions) {
        RetireVoiceSubscriptionPeer719(previousSubscription);
    }
    if (!previousSessions.empty()) {
        Log("voice",
            "Replaced publisher userId=%lld senderContext=0x%llx oldPublishers=%zu rebuiltSubscriptions=%zu",
            static_cast<long long>(active->userId),
            static_cast<unsigned long long>(active->senderContext),
            previousSessions.size(), replacedSubscriptions.size());
    }
    Log("voice",
        "Registered publisher participantId=%lld userId=%lld feedId=%lld ssrc=%u session=%s senderContext=0x%llx",
        static_cast<long long>(active->participantId),
        static_cast<long long>(active->userId),
        static_cast<long long>(active->feedId), active->ssrc,
        active->sessionId.c_str(),
        static_cast<unsigned long long>(active->senderContext));

    // Publisher registration is the authoritative topology change. Previously
    // subscription creation happened only when a later mute-state or
    // subscription-ACK event happened to arrive. The removed pass-through
    // diagnostics changed that timing and exposed the race for an already-muted
    // remote Player, which never sent another PublishStateChange.
    EnsureVoiceSubscriptions719();
}
#else
static void HandleRelayCandidatesGathered719(
    void* rawArguments, uint64_t) {
    RelayCandidatesGathered719 decoded;
    DecodeRelayCandidatesGathered719(rawArguments, decoded);
}

static void HandleSubscriptionHandshakeAcked719(
    uint8_t*, void* rawArguments, uint64_t) {
    SubscriptionHandshakeAcked719 decoded;
    DecodeSubscriptionHandshakeAcked719(rawArguments, decoded);
}

static void HandlePublishStateChange719(
    uint8_t*, void* rawArguments, uint64_t) {
    int muteState = 0;
    std::string sessionId;
    DecodePublishStateChange719(rawArguments, muteState, sessionId);
}

static void HandlePublishingHandshakeInitiated719(
    uint8_t*, void* rawArguments, uint64_t) {
    std::string decodedOffer;
    std::string sessionId;
    DecodePublishingHandshakeInitiated719(
        rawArguments, decodedOffer, sessionId);
}
#endif

static __int64 ProcessVoiceRemoteEventHook(
    uint8_t* voiceChatService, void* eventDescriptor,
    void* arguments, uint64_t senderContext) {
    const auto* imageBase = reinterpret_cast<const uint8_t*>(GetModuleHandleW(nullptr));
    const char* eventName = nullptr;
    bool shouldSendJoinedVoice = false;
    bool shouldSendReJoinedVoice = false;
    bool shouldSendUserTurnAuth = false;
    bool shouldHandlePublishState = false;
    bool shouldHandlePublishingHandshake = false;
    bool shouldHandleRelayCandidates = false;
    bool shouldHandleSubscriptionHandshakeAck = false;
    bool shouldLogSubscriptionFeedStarted = false;
    uintptr_t eventDescriptorRva = 0;
    if (imageBase) {
        eventDescriptorRva = reinterpret_cast<uintptr_t>(eventDescriptor) -
            reinterpret_cast<uintptr_t>(imageBase);
        if (eventDescriptor == static_cast<const void*>(
                imageBase + kClientVoiceCapabilityDescriptorRva)) {
            eventName = "VoiceChatClientVoiceCapability";
            shouldSendJoinedVoice = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kClientVoiceCapabilityWithConfigDescriptorRva)) {
            eventName = "VoiceChatClientVoiceCapabilityWithConfig";
            shouldSendJoinedVoice = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kClientRetryJoinWithConfigDescriptorRva)) {
            eventName = "ClientRetryJoinWithConfig";
            shouldSendReJoinedVoice = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kPublishStateChangeDescriptorRva)) {
            eventName = "PublishStateChange";
            shouldHandlePublishState = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kUpdateTurnAuthInfoRequestDescriptorRva)) {
            eventName = "UpdateTurnAuthInfoRequest";
            shouldSendUserTurnAuth = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kPublishingHandshakeInitiatedDescriptorRva)) {
            eventName = "PublishingHandshakeInitiated";
            shouldHandlePublishingHandshake = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kRelayCandidatesGatheredDescriptorRva)) {
            eventName = "RelayCandidatesGathered";
            shouldHandleRelayCandidates = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kSubscriptionHandshakeAckedDescriptorRva)) {
            eventName = "SubscriptionHandshakeAcked";
            shouldHandleSubscriptionHandshakeAck = true;
        } else if (eventDescriptor == static_cast<const void*>(
                       imageBase + kSubscriptionFeedStartedDescriptorRva)) {
            eventName = "SubscriptionFeedStarted";
            shouldLogSubscriptionFeedStarted = true;
        }
    }

    if (eventName) {
        void* controlPlane = voiceChatService
            ? *reinterpret_cast<void**>(voiceChatService + 1128)
            : nullptr;
        Log("voice",
            "processRemoteEvent received %s service=%p controlPlane=%p arguments=%p senderContext=0x%llx",
            eventName, voiceChatService, controlPlane, arguments,
            static_cast<unsigned long long>(senderContext));
    } else {
        Log("voice",
            "processRemoteEvent received unclassified descriptorRva=0x%llx service=%p arguments=%p senderContext=0x%llx",
            static_cast<unsigned long long>(eventDescriptorRva),
            voiceChatService, arguments,
            static_cast<unsigned long long>(senderContext));
    }

    // 0.719's own server-to-client mute propagation dispatches while the incoming
    // RemoteEventSource is still active, before forwarding to the inherited
    // processRemoteEvent implementation. Match that ordering: the inherited
    // receiver can consume/invalidate target state associated with a4.
    if (shouldSendJoinedVoice || shouldSendReJoinedVoice ||
        shouldSendUserTurnAuth) {
        const VoiceParticipantIdentity719 identity = shouldSendReJoinedVoice
            ? RotateVoiceParticipantIdentity719(senderContext)
            : GetOrCreateVoiceParticipantIdentity719(senderContext);
        if (shouldSendJoinedVoice)
            SendJoinedVoice719(
                voiceChatService, senderContext, identity);
        if (shouldSendReJoinedVoice)
            SendJoinedVoice719(
                voiceChatService, senderContext, identity, true);
        if (shouldSendUserTurnAuth)
            SendUserTurnAuth719(
                voiceChatService, senderContext, identity.sessionId);
    }
    if (shouldHandlePublishState)
        HandlePublishStateChange719(
            voiceChatService, arguments, senderContext);
    if (shouldHandlePublishingHandshake)
        HandlePublishingHandshakeInitiated719(
            voiceChatService, arguments, senderContext);
    if (shouldHandleRelayCandidates)
        HandleRelayCandidatesGathered719(arguments, senderContext);
    if (shouldHandleSubscriptionHandshakeAck)
        HandleSubscriptionHandshakeAcked719(
            voiceChatService, arguments, senderContext);
    if (shouldLogSubscriptionFeedStarted)
        LogSubscriptionFeedStarted719(arguments);
    return gOrigProcessVoiceRemoteEvent(
        voiceChatService, eventDescriptor, arguments, senderContext);
}

static bool InstallProcessVoiceRemoteEventHook() {
    // VoiceChatService::processRemoteEvent in exact Studio 0.719. This is the
    // authoritative server-side point for the Player's capability, retry, and
    // TURN-auth events. The hook supplies the local control-plane responses and
    // drives publishing/subscription WebRTC sessions.
    gProcessVoiceRemoteEvent = reinterpret_cast<types::process_voice_remote_event_fn>(
        ScanAny("voice.processRemoteEvent", {
            "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC 80 00 00 00 4D 8B E1 4D 8B F8 48 8B FA",
        }));
    if (!gProcessVoiceRemoteEvent) {
        Log("voice", "processRemoteEvent pattern not found -- local voice control plane is unavailable");
        return false;
    }

    const MH_STATUS status = MH_CreateHook(
        reinterpret_cast<LPVOID>(gProcessVoiceRemoteEvent),
        reinterpret_cast<LPVOID>(&ProcessVoiceRemoteEventHook),
        reinterpret_cast<LPVOID*>(&gOrigProcessVoiceRemoteEvent));
    if (status != MH_OK) {
        Log("voice", "MH_CreateHook(processRemoteEvent) failed: %d",
            static_cast<int>(status));
        gProcessVoiceRemoteEvent = nullptr;
        gOrigProcessVoiceRemoteEvent = nullptr;
        return false;
    }

    Log("voice", "Hooked the 0.719 local voice control plane");
    return true;
}

void VoiceChat719::InstallHooks() {
    if (!InstallVoiceRccHook())
        Log("main", "Could not enable the 0.719 VoiceChatService RCC gates");
    if (!InstallVoiceRccContextGateHook())
        Log("main", "Could not override the 0.719 ConfigureWithoutRun voice context gate");
    if (!InstallProcessVoiceRemoteEventHook())
        Log("main", "Could not install the 0.719 incoming voice-event response hook");
}

void VoiceChat719::RemoveHooks() {
    struct HookTarget {
        const char* label;
        LPVOID address;
    };
    const HookTarget hooks[] = {
        {"voice.processRemoteEvent", reinterpret_cast<LPVOID>(gProcessVoiceRemoteEvent)},
        {"voice.contextGate", reinterpret_cast<LPVOID>(gVoiceRccContextGate)},
        {"voice.configureWithoutRun", reinterpret_cast<LPVOID>(gConfigureWithoutRun)},
    };

    for (const HookTarget& hook : hooks) {
        if (!hook.address)
            continue;

        const MH_STATUS disableStatus = MH_DisableHook(hook.address);
        if (disableStatus != MH_OK && disableStatus != MH_ERROR_DISABLED &&
            disableStatus != MH_ERROR_NOT_CREATED) {
            Log("main", "DisableHook(%s) during rollback failed: %d",
                hook.label, static_cast<int>(disableStatus));
        }

        const MH_STATUS removeStatus = MH_RemoveHook(hook.address);
        if (removeStatus != MH_OK && removeStatus != MH_ERROR_NOT_CREATED) {
            Log("main", "RemoveHook(%s) during rollback failed: %d",
                hook.label, static_cast<int>(removeStatus));
        }
    }
}
