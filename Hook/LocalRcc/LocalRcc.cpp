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
// File: LocalRcc.cpp
// Description: (Disclaimer - This code was slopped together by Claude Opus 4.8 and GPT 5.6 Sol.)
// Port of rsblox/local_rcc (https://github.com/rsblox/local_rcc) into the
//              noobWarrior Hook system. Loaded by noobhook.dll only when running inside
//              RobloxStudioBeta.exe -- it patches Studio so that Player can connect to
//              a Studio-hosted team test server.
// Exact-version voice support lives in VoiceChat719.cpp.
//
// Original work by 7ap & Epix (https://github.com/rsblox). Ported to MinHook +
// Hooking.Patterns.

#include <windows.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>
#include <string>
#include <vector>

#include <Luau/Bytecode.h>
#include <Luau/BytecodeBuilder.h>
#if !defined(NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER)
#include <Luau/BytecodeUtils.h>
#endif
#include <Luau/Compiler.h>

#include <Hooking.Patterns.h>
#include <MinHook.h>

#include <blake3.h>

#define XXH_INLINE_ALL
#include <xxhash.h>
#include <zstd.h>

#include "LocalRccShared.h"
#include "VoiceChat719.h"

using LocalRccShared::Log;
using LocalRccShared::ScanAny;

// -----------------------------------------------------------------------------
// Logging -- own log file so it doesn't interleave with noobhook.log
// -----------------------------------------------------------------------------

static FILE* gLog = nullptr;
static HMODULE gLocalRccModule = nullptr;
static volatile LONG gInitializationState = 0;
static volatile LONG gInitializationResult = ERROR_DLL_INIT_FAILED;

void LocalRccShared::Log(const char* category, const char* format, ...) {
    if (!gLog) return;
    fprintf(gLog, "[LocalRcc::%s] ", category);
    va_list args;
    va_start(args, format);
    vfprintf(gLog, format, args);
    va_end(args);
    fprintf(gLog, "\n");
    fflush(gLog);
}

// -----------------------------------------------------------------------------
// Roblox-allocator-backed operator new/delete.
//
// Any std::string we return from compile_hook ends up owned by Roblox code; the
// pointer it holds must therefore have been produced by Roblox's allocator, not
// our CRT's. Studio exports rbxAllocate / rbxDeallocate exactly for this; we
// route global new/delete in *this DLL only* through those exports.
//
// This is also why LocalRcc lives in its own DLL: noobhook.dll runs inside
// Player and RCCService too, where these exports don't exist.
// -----------------------------------------------------------------------------

namespace {
using rbx_alloc_fn = void* (*)(size_t);
using rbx_dealloc_fn = void (*)(void*);

rbx_alloc_fn ResolveAllocate() {
    static rbx_alloc_fn fn = reinterpret_cast<rbx_alloc_fn>(
        GetProcAddress(GetModuleHandleA(nullptr), "rbxAllocate"));
    return fn;
}

rbx_dealloc_fn ResolveDeallocate() {
    static rbx_dealloc_fn fn = reinterpret_cast<rbx_dealloc_fn>(
        GetProcAddress(GetModuleHandleA(nullptr), "rbxDeallocate"));
    return fn;
}
} // namespace

void* operator new(size_t size)   { return ResolveAllocate()(size); }
void* operator new[](size_t size) { return ResolveAllocate()(size); }
void operator delete(void* p) noexcept   { if (p) ResolveDeallocate()(p); }
void operator delete[](void* p) noexcept { if (p) ResolveDeallocate()(p); }
void operator delete(void* p, size_t) noexcept   { if (p) ResolveDeallocate()(p); }
void operator delete[](void* p, size_t) noexcept { if (p) ResolveDeallocate()(p); }

// -----------------------------------------------------------------------------
// Target function typedefs & cached addresses
// -----------------------------------------------------------------------------

namespace types {
    using compile_fn         = std::string (*)(const std::string& source, int target, int options);
    using deserialize_item_fn = void* (*)(void* self, void* result, void* in_bitstream, int item_type);

    enum network_value_format { protected_string_bytecode = 0x2f };
}

static types::compile_fn          gCompile               = nullptr;
static types::compile_fn          gOrigCompile           = nullptr;
static types::deserialize_item_fn gDeserializeItemOuter  = nullptr;
static types::deserialize_item_fn gOrigDeserializeOuter  = nullptr;
static types::deserialize_item_fn gDeserializeItemInner  = nullptr;
static types::deserialize_item_fn gOrigDeserializeInner  = nullptr;
static uintptr_t                  gTypeForPropertyImm     = 0;

namespace {
constexpr DWORD kStudio719TimeDateStamp = 0x0c6a7690;
constexpr DWORD kStudio719SizeOfImage = 0x0ccff000;
constexpr DWORD kStudio574TimeDateStamp = 0xc1b36571;
constexpr DWORD kStudio574SizeOfImage = 0x06b42000;

struct StudioImageIdentity {
    uint8_t* base;
    DWORD timeDateStamp;
    DWORD sizeOfImage;
    bool valid;
};

StudioImageIdentity GetStudioImageIdentity() {
    StudioImageIdentity identity {};
    HMODULE module = GetModuleHandleW(nullptr);
    if (!module)
        return identity;

    auto* base = reinterpret_cast<uint8_t*>(module);
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        dos->e_lfanew > 0x100000) {
        return identity;
    }

    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < sizeof(IMAGE_DOS_HEADER)) {
        return identity;
    }

    identity.base = base;
    identity.timeDateStamp = nt->FileHeader.TimeDateStamp;
    identity.sizeOfImage = nt->OptionalHeader.SizeOfImage;
    identity.valid = true;
    return identity;
}

bool IsExactStudio719(const StudioImageIdentity& identity) {
    return identity.valid &&
        identity.timeDateStamp == kStudio719TimeDateStamp &&
        identity.sizeOfImage == kStudio719SizeOfImage;
}

bool IsExactStudio574(const StudioImageIdentity& identity) {
    return identity.valid &&
        identity.timeDateStamp == kStudio574TimeDateStamp &&
        identity.sizeOfImage == kStudio574SizeOfImage;
}

bool IsStudio574CompileCaller(const StudioImageIdentity& identity,
                              const void* returnAddress) {
    if (!IsExactStudio574(identity) || !returnAddress)
        return false;

    // Studio 0.574 folds LuaVM::compile's empty-string stub together with
    // unrelated functions that have different signatures. Hooking the shared
    // body therefore also intercepts calls which only provide the hidden
    // std::string return pointer; treating their garbage RDX value as `source`
    // crashes before Studio finishes starting. These are the direct callers
    // that actually populate the compile arguments in this exact image.
    constexpr std::array<uintptr_t, 6> kCompileReturnRvas = {
        0x00a9b9c5,
        0x00a9bb07,
        0x00d77a5a,
        0x01292812,
        0x01292881,
        0x0232dab0,
    };

    const uintptr_t returnRva =
        reinterpret_cast<uintptr_t>(returnAddress) -
        reinterpret_cast<uintptr_t>(identity.base);
    for (const uintptr_t compileReturnRva : kCompileReturnRvas) {
        if (returnRva == compileReturnRva)
            return true;
    }
    return false;
}

} // namespace

std::uint8_t* LocalRccShared::GetExactStudio719ImageBase() {
    const StudioImageIdentity identity = GetStudioImageIdentity();
    return IsExactStudio719(identity) ? identity.base : nullptr;
}

#if defined(NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER)
constexpr uint8_t kExpectedBytecodeVersion = 3;
#else
constexpr uint8_t kExpectedBytecodeVersion = 6;
#endif
static_assert(LBC_VERSION_TARGET == kExpectedBytecodeVersion,
    "Selected Luau tag does not emit the bytecode version expected by this LocalRcc backend");

// -----------------------------------------------------------------------------
// Hook: LuaVM::compile
// -----------------------------------------------------------------------------

namespace {
constexpr size_t kLegacyLsbTrailerSize = 40;
constexpr std::array<uint8_t, 4> kLegacyLsbMarker = {
    0x32, 0xc4, 0x6a, 0x94
};
constexpr std::array<uint8_t, 4> kLegacyLsbKey = {
    0x4E, 0x4F, 0x4F, 0x42
};

uint8_t RotateLeft8(uint8_t value, unsigned count) {
    count &= 7;
    if (count == 0)
        return value;
    return static_cast<uint8_t>((value << count) | (value >> (8 - count)));
}

uint8_t LegacyLsbSignatureByte(size_t index, uint8_t key, uint8_t digest) {
    const unsigned lane = static_cast<unsigned>(index & 3);
    const unsigned keyRotation = static_cast<unsigned>((key + index) & 3);
    if (lane == 0 || lane == 2) {
        const uint8_t mixed = static_cast<uint8_t>(digest ^ static_cast<uint8_t>(~key));
        return RotateLeft8(mixed, keyRotation + (lane == 0 ? 1 : 3));
    }

    const uint8_t mixed = static_cast<uint8_t>(key ^ static_cast<uint8_t>(~digest));
    return RotateLeft8(mixed, keyRotation + (lane == 1 ? 2 : 4));
}

bool AppendLegacyLsbTrailer(std::string& bytecode) {
    std::array<uint8_t, BLAKE3_OUT_LEN> digest {};
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, bytecode.data(), bytecode.size());
    blake3_hasher_finalize(&hasher, digest.data(), digest.size());

    std::array<uint8_t, 32> sig {};
    for (size_t i = 0; i < sig.size(); ++i)
        sig[i] = LegacyLsbSignatureByte(i, kLegacyLsbKey[i & 3], digest[i]);

    std::array<uint8_t, kLegacyLsbTrailerSize> trailer {};
    for (size_t k = 0; k < 4; ++k)
        trailer[k] = static_cast<uint8_t>(sig[k] ^ kLegacyLsbMarker[k]);
    for (size_t k = 0; k < 4; ++k)
        trailer[4 + k] = static_cast<uint8_t>(sig[k] ^ kLegacyLsbKey[k]);
    std::memcpy(trailer.data() + 8, sig.data(), 16);        // trailer[8..23]  = sig[0..15]
    std::memcpy(trailer.data() + 24, sig.data() + 16, 16);  // trailer[24..39] = sig[16..31]

    bytecode.append(reinterpret_cast<const char*>(trailer.data()), trailer.size());
    return true;
}

bool ValidateRsb1RoundTrip(const std::vector<uint8_t>& encrypted, size_t encryptedSize,
                           const uint8_t key[4], XXH32_hash_t expectedHash,
                           const std::string& bytecode) {
    std::vector<uint8_t> plain(encrypted.begin(), encrypted.begin() + encryptedSize);
    for (size_t i = 0; i < plain.size(); ++i)
        plain[i] ^= uint8_t(key[i & 3] + (i * 41));

    if (XXH32(plain.data(), plain.size(), 42) != expectedHash || plain.size() < 8 ||
        std::memcmp(plain.data(), "RSB1", 4) != 0) {
        return false;
    }

    uint32_t rawSize = 0;
    std::memcpy(&rawSize, plain.data() + 4, sizeof(rawSize));
    if (rawSize != bytecode.size())
        return false;

    std::vector<uint8_t> decompressed(rawSize);
    const size_t decompressedSize = ZSTD_decompress(
        decompressed.data(), decompressed.size(), plain.data() + 8, plain.size() - 8);
    if (ZSTD_isError(decompressedSize) || decompressedSize != rawSize ||
        std::memcmp(decompressed.data(), bytecode.data(), rawSize) != 0) {
        return false;
    }

    return true;
}
} // namespace

static std::string CompileHook(const std::string& source, int target, int options) {
    const StudioImageIdentity identity = GetStudioImageIdentity();
    const void* returnAddress = _ReturnAddress();
    if (IsExactStudio574(identity) &&
        !IsStudio574CompileCaller(identity, returnAddress)) {
        static std::atomic<bool> reportedFoldedAlias {false};
        if (!reportedFoldedAlias.exchange(true, std::memory_order_relaxed)) {
            Log("compile", "Bypassing folded non-compiler caller at Studio RVA 0x%zx",
                reinterpret_cast<uintptr_t>(returnAddress) -
                    reinterpret_cast<uintptr_t>(identity.base));
        }

        // The original folded stub only initializes the hidden std::string
        // return object and does not read source/target/options, so forwarding
        // the untouched registers is safe even for the aliased signatures.
        return gOrigCompile(source, target, options);
    }

    Log("compile", "LuaVM::compile(source=%zu bytes, target=%d, options=%d)",
        source.size(), target, options);

    if (source.empty()) {
        Log("compile", "Empty source, returning empty");
        return std::string();
    }

    // Luau passes the complete instruction array to the encoder. The wire opcode
    // cipher is the linear op*227 (the client decode composes to op*203). This is
    // correct for a self-consistent build -- Studio host, Player joiner and pinned
    // Luau all the SAME version. AUX words are skipped
    // via getOpLength. The 0.574 legacy path also uses op*227.
    class BytecodeEncoderClient : public Luau::BytecodeEncoder {
#if defined(NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER)
        uint8_t encodeOp(uint8_t op) override {
            return uint8_t(op * 227);
        }
#else
        void encode(uint32_t* data, size_t count) override {
            for (size_t i = 0; i < count;) {
                const uint8_t op = LUAU_INSN_OP(data[i]);
                data[i] = uint8_t(op * 227) | (data[i] & ~0xffu);
                i += Luau::getOpLength(LuauOpcode(op));
            }
        }
#endif
    };

    static const char* kSpecialGlobals[] = {
        "game", "Game", "workspace", "Workspace",
        "script", "shared", "plugin",
        nullptr
    };

    Luau::CompileOptions opts {};
    opts.optimizationLevel = 2;
    opts.debugLevel        = 1;
    opts.vectorLib         = "Vector3";
    opts.vectorCtor        = "new";
#if !defined(NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER)
    opts.vectorType        = "Vector3";
#endif
    opts.mutableGlobals    = kSpecialGlobals;

    BytecodeEncoderClient encoder;
#if !defined(NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER)
    static std::atomic<bool> reportedEncoding {false};
    if (!reportedEncoding.exchange(true, std::memory_order_relaxed))
        Log("compile", "Encoding Player %s wire opcodes as op*227 (self-consistent build)",
            NOOBWARRIOR_LOCALRCC_LUAU_VERSION);
#endif
    std::string bytecode = Luau::compile(source, opts, {}, &encoder);
    if (bytecode.empty() || bytecode[0] == '\0') {
        Log("compile", "Compile error (first byte=0, size=%zu)", bytecode.size());
        // Luau's bytecode-error string is already in the form expected by the
        // caller. Wrapping it in RSB1 turns a compile error into "bytecode
        // corrupted" on Player, which hides the actual script error.
        return bytecode;
    }
    const uint8_t bytecodeVersion = static_cast<uint8_t>(bytecode[0]);
    if (bytecodeVersion != kExpectedBytecodeVersion) {
        Log("compile", "Compiler emitted bytecode version %u; expected %u -- refusing corrupt wire payload",
            static_cast<unsigned>(bytecodeVersion),
            static_cast<unsigned>(kExpectedBytecodeVersion));
        return std::string();
    }
    Log("compile", "Compile OK (bytecode=%zu bytes, version=%u)",
        bytecode.size(), static_cast<unsigned>(bytecodeVersion));

    if (IsExactStudio719(identity)) {
        const size_t unsignedSize = bytecode.size();
        if (!AppendLegacyLsbTrailer(bytecode))
            return std::string();
        Log("compile", "Appended Player 0.719 scheme-0 BLAKE3 trailer "
            "(unsigned=%zu, signed=%zu, +%zu)",
            unsignedSize, bytecode.size(), kLegacyLsbTrailerSize);
    }

    const size_t compressedCapacity = ZSTD_compressBound(bytecode.size());
    const uint32_t uncompressedSize = static_cast<uint32_t>(bytecode.size());

    // [magic 4] [uncompressed_size 4] [compressed payload]
    std::vector<uint8_t> encoded(4 + 4 + compressedCapacity);
    std::memcpy(&encoded[0], "RSB1", 4);
    std::memcpy(&encoded[4], &uncompressedSize, 4);

    const size_t compressedSize = ZSTD_compress(
        &encoded[8], compressedCapacity,
        bytecode.data(), bytecode.size(),
        ZSTD_maxCLevel());
    if (ZSTD_isError(compressedSize)) {
        Log("compile", "ZSTD_compress failed: %s", ZSTD_getErrorName(compressedSize));
        return std::string();
    }

    const size_t encodedSize = 4 + 4 + compressedSize;
    const XXH32_hash_t hash = XXH32(encoded.data(), encodedSize, 42);
    uint8_t key[4];
    std::memcpy(key, &hash, 4);
    for (size_t i = 0; i < encodedSize; i++)
        encoded[i] ^= uint8_t(key[i % 4] + (i * 41));

    if (!ValidateRsb1RoundTrip(encoded, encodedSize, key, hash, bytecode)) {
        Log("compile", "Internal raw-bytecode RSB1 round-trip validation failed");
        return std::string();
    }

    static std::atomic<bool> reportedValidation {false};
    if (!reportedValidation.exchange(true, std::memory_order_relaxed)) {
        Log("compile", "Raw-bytecode RSB1 round-trip OK (bytecode=%zu, wire=%zu, xxh32=%08x)",
            bytecode.size(), encodedSize, static_cast<unsigned>(hash));
    }

    return std::string(reinterpret_cast<const char*>(encoded.data()), encodedSize);
}

static void* DeserializeItemOuterHook(void* self, void* result, void* inBitstream, int itemType) {
    return gOrigDeserializeOuter(self, result, inBitstream, itemType);
}

static bool UsesInnerDeserializer(int itemType) {
    switch (itemType) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 19:
    case 21:
    case 35:
        return true;
    default:
        return false;
    }
}

static void* DeserializeItemInnerHook(void* self, void* result, void* inBitstream, int itemType) {
    if (UsesInnerDeserializer(itemType))
        return gOrigDeserializeInner(self, result, inBitstream, itemType);

    return DeserializeItemOuterHook(self, result, inBitstream, itemType);
}

namespace offline {
    using from_components_fn = void  (*)(void* res16, void* schema, void* host, void* path, void* query, void* fragment);
    using trust_check_fn     = uint64_t* (*)(const char* url, char a2, char a3);
    using not_trusted_fn     = char* (*)(void* a1, void* a2);
}

static offline::from_components_fn gFromComponents     = nullptr;
static offline::from_components_fn gOrigFromComponents = nullptr;
static offline::trust_check_fn     gTrustCheck         = nullptr;
static offline::trust_check_fn     gOrigTrustCheck     = nullptr;
static offline::not_trusted_fn     gNotTrusted         = nullptr;
static offline::not_trusted_fn     gOrigNotTrusted     = nullptr;

// `schema` and `host` are pointers to std::string-shaped things laid out as
// {data ptr @ +0, length @ +8}. Studio-offline relies on the original strings
// being heap-allocated (long enough to break SSO); short literals like "http"
// would normally be SSO. In practice the call sites here pass strings whose
// data pointer at +0 IS what gets used downstream -- the layout assumption
// holds for these specific call paths in 0.574.
//
// The host includes ":8080" so the URL stays self-contained: WebView2 spawns
// its own subprocesses for navigation and those don't inherit Hook.cpp's
// connect() redirect (port 80 -> emulator). Embedding the emulator's HTTP
// port in the host means WebView2 (and any other consumer) lands on the
// emulator directly without needing per-process injection or a port-80 bind.
// Hook.cpp's redirect is still useful for code paths that bypass
// FromComponents and connect to localhost:80 by other means.
static void FromComponentsHook(void* res16, void* schema, void* host,
                               void* path, void* query, void* fragment) {
    static char kHostBuffer[32] = {0};
    static bool kHostInit = false;
    if (!kHostInit) {
        kHostInit = true;
        uint16_t port = 8080;
        char portBuf[16] = {0};
        if (GetEnvironmentVariableA("NOOBHOOK_HTTP_PORT", portBuf, sizeof(portBuf)) > 0) {
            int p = atoi(portBuf);
            if (p > 0 && p < 65536) port = static_cast<uint16_t>(p);
        }
        _snprintf_s(kHostBuffer, sizeof(kHostBuffer), _TRUNCATE, "localhost:%u", (unsigned)port);
    }
    static const char* kScheme = "http";

    *reinterpret_cast<const char**>(host) = kHostBuffer;
    *reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(host) + 8) = std::strlen(kHostBuffer);

    *reinterpret_cast<const char**>(schema) = kScheme;
    *reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(schema) + 8) = std::strlen(kScheme);

    gOrigFromComponents(res16, schema, host, path, query, fragment);
}

static uint64_t* TrustCheckHook(const char* url, char a2, char a3) {
    if (url && a3 == 0 && std::strstr(url, "http://localhost")) {
        // Original trust check accepts roblox.com; pretend that's what we got.
        return gOrigTrustCheck("http://roblox.com", a2, a3);
    }
    return gOrigTrustCheck(url, a2, a3);
}

static char* NotTrustedHook(void* /*a1*/, void* /*a2*/) {
    // Caller treats this as a small status string ("1" == ok). Static buffer
    // so the pointer survives return (original code returned a stack array,
    // which was UB; we keep the value, drop the UB).
    static char kOne[2] = { '1', '\0' };
    return kOne;
}

static bool InstallOfflineHooks() {
    gFromComponents = reinterpret_cast<offline::from_components_fn>(ScanAny("offline.fromComponents", {
        "48 89 5C 24 18 4C 89 4C 24 20 48 89 4C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60",  // 0.574
        "48 89 5C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4D 8B F9 49 8B D8",
    }));
    gTrustCheck = reinterpret_cast<offline::trust_check_fn>(ScanAny("offline.trustCheck", {
        "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 56 41 57 48 8D AC 24 10 FF FF FF 48 81 EC F0 01 00 00 45",  // 0.574
        "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 4C 89 64 24 ? 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 0F B6 E1 45 0F B6 F8",  // 0.719
    }));
    gNotTrusted = reinterpret_cast<offline::not_trusted_fn>(ScanAny("offline.httpReqNotTrusted", {
        "48 89 74 24 10 57 48 83 EC 40 48 8B FA 48 8B F1 E8",
        "48 89 74 24 ? 57 48 83 EC ? 48 8B FA 48 8B F1 E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B C8",
    }));

    // All three are independent -- install whichever matched. Missing one just
    // means that codepath stays unhooked (Studio may still mostly work).
    auto install = [](const char* label, void* target, void* detour, void** orig) {
        if (!target) return;
        MH_STATUS st = MH_CreateHook(target, detour, orig);
        if (st != MH_OK) Log("offline", "MH_CreateHook(%s) failed: %d", label, (int)st);
        else             Log("offline", "Hooked %s", label);
    };
    install("FromComponents", (void*)gFromComponents, (void*)&FromComponentsHook, (void**)&gOrigFromComponents);
    install("TrustCheck",     (void*)gTrustCheck,     (void*)&TrustCheckHook,     (void**)&gOrigTrustCheck);
    install("NotTrusted",     (void*)gNotTrusted,     (void*)&NotTrustedHook,     (void**)&gOrigNotTrusted);
    return gFromComponents || gTrustCheck || gNotTrusted;
}

// -----------------------------------------------------------------------------
// Patch: StudioCookieManager security-cookie check -> always "proceed"
//
// WebView2 is the Studio login surface, and it only proceeds once
// StudioCookieManager decides the security cookie is valid -- the codepath logged
// as "[FLog::StudioCookieManager] Security cookie is cached so we proceed saving
// now." Running offline against the emulator, that check takes the reject branch
// and login stalls, so the studio-offline URL hooks alone are not enough. This is
// a port of studio-offline's cookie patch:
// flip the `jz` guarding that FLog path to `jnz`.
//
// studio-offline finds it by string xref: locate the FLog literal, find its lone
// `lea`, then walk back to the `jz` after a `cmp`. On both 0.712 and 0.729 that
// `jz` sits 14 bytes before the lea, at the head of a version-stable compare
// epilogue -- `jz(74 74) ; cmp al,6 ; jb ; shr rax,8 ; cmp al,4 ; jb ; lea` -- so
// we anchor on that epilogue directly (unique single hit on both builds:
// .text+0x218092f in 0.712, .text+0x24ff2c1 in 0.729). The leading 74 IS the jz.
// -----------------------------------------------------------------------------

static void PatchSecurityCookie() {
    auto* jz = reinterpret_cast<uint8_t*>(ScanAny("securityCookie.jz", {
        "74 74 3C 06 72 5C 48 C1 E8 08 3C 04 72 54 48 8D 05",
    }));
    if (!jz) {
        Log("cookie", "security-cookie jz pattern not found -- WebView2 login may stall offline");
        return;
    }
    if (jz[0] != 0x74) {
        Log("cookie", "expected jz (0x74) at %p, got 0x%02x -- not patching", jz, jz[0]);
        return;
    }
    DWORD oldProtect;
    if (!VirtualProtect(jz, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("cookie", "VirtualProtect failed @ %p", jz);
        return;
    }
    jz[0] = 0x75; // jz -> jnz
    VirtualProtect(jz, 1, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), jz, 1);
    Log("cookie", "patched StudioCookieManager security-cookie jz->jnz @ %p", jz);
}

// -----------------------------------------------------------------------------
// Patch: NetworkSchema::typeForProperty
// Force ProtectedString columns to serialize as ProtectedStringBytecode.
// -----------------------------------------------------------------------------

struct TypeForPropertyPatchState {
    uint8_t originalByte = 0;
    DWORD originalProtection = 0;
    bool applied = false;
};

static bool RestoreTypeForProperty(TypeForPropertyPatchState& state) {
    if (!state.applied)
        return true;

    auto* address = reinterpret_cast<uint8_t*>(gTypeForPropertyImm);
    DWORD currentProtection = 0;
    if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &currentProtection)) {
        Log("schema", "VirtualProtect failed while restoring typeForProperty @ %p",
            address);
        return false;
    }

    *address = state.originalByte;
    FlushInstructionCache(GetCurrentProcess(), address, 1);
    const bool verified =
        *reinterpret_cast<volatile const uint8_t*>(address) == state.originalByte;

    DWORD ignored = 0;
    const bool protectionRestored = VirtualProtect(
        address, 1, state.originalProtection, &ignored) != FALSE;
    if (verified)
        state.applied = false;

    if (!verified || !protectionRestored) {
        Log("schema", "typeForProperty rollback failed (protect=%s value=%02x expected=%02x)",
            protectionRestored ? "ok" : "FAILED",
            static_cast<unsigned>(*reinterpret_cast<volatile const uint8_t*>(address)),
            static_cast<unsigned>(state.originalByte));
        return false;
    }

    Log("schema", "Restored typeForProperty byte @ %p to %02x", address,
        static_cast<unsigned>(state.originalByte));
    return true;
}

static bool PatchTypeForProperty(TypeForPropertyPatchState& state) {
    if (!gTypeForPropertyImm)
        return false;

    auto* address = reinterpret_cast<uint8_t*>(gTypeForPropertyImm);
    const uint8_t byte = static_cast<uint8_t>(types::protected_string_bytecode);
    state = {};
    state.originalByte = *reinterpret_cast<volatile const uint8_t*>(address);

    if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE,
                        &state.originalProtection)) {
        Log("schema", "VirtualProtect failed for typeForProperty @ %p",
            address);
        return false;
    }

    *address = byte;
    state.applied = true;
    FlushInstructionCache(GetCurrentProcess(), address, 1);
    const bool verified = *reinterpret_cast<volatile const uint8_t*>(address) == byte;

    DWORD ignored = 0;
    const bool protectionRestored = VirtualProtect(
        address, 1, state.originalProtection, &ignored) != FALSE;
    if (!protectionRestored || !verified) {
        Log("schema", "typeForProperty patch verification failed (restore=%s value=%02x)",
            protectionRestored ? "ok" : "FAILED",
            static_cast<unsigned>(*reinterpret_cast<volatile const uint8_t*>(address)));
        if (!RestoreTypeForProperty(state))
            Log("schema", "CRITICAL: could not roll back failed typeForProperty patch");
        return false;
    }
    return true;
}

static void RemoveTeamTestHooks() {
#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
    VoiceChat719::RemoveHooks();
#endif

    struct HookTarget {
        const char* label;
        LPVOID address;
    };
    const HookTarget hooks[] = {
        {"deserializeItem.inner", reinterpret_cast<LPVOID>(gDeserializeItemInner)},
        {"deserializeItem.outer", reinterpret_cast<LPVOID>(gDeserializeItemOuter)},
        {"compile", reinterpret_cast<LPVOID>(gCompile)},
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

// -----------------------------------------------------------------------------
// Patch: modern key-exchange result -> take the accepted path
//
// Modern Studio compares the negotiated key-exchange result against the stored
// session mode and conditionally enters the accepted path.
// -----------------------------------------------------------------------------

static void PatchModernKeyExchange() {
    auto* anchor = reinterpret_cast<uint8_t*>(ScanAny("keyExchange.accept", {
        "0F B6 8F 50 0A 00 00 3A C8 74 51",
    }));
    if (!anchor) {
        Log("keyexchange", "accept-branch pattern not found -- leaving key exchange unchanged");
        return;
    }

    uint8_t* branch = anchor + 9;
    if (branch[0] != 0x74 || branch[1] != 0x51) {
        Log("keyexchange", "unexpected branch bytes @ %p (%02x %02x) -- not patching",
            branch, branch[0], branch[1]);
        return;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(branch, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("keyexchange", "VirtualProtect failed @ %p", branch);
        return;
    }
    branch[0] = 0xEB; // jz -> jmp
    VirtualProtect(branch, 1, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), branch, 1);
    Log("keyexchange", "patched key-exchange accept branch @ %p", branch);
}

// -----------------------------------------------------------------------------
// Patch: RakNet::RakPeer::verifyPreauthMac -> always "allow"
//
// Roblox gates every incoming connection on a "preauth MAC" -- a keyed MAC over
// the open-connection handshake, checked host-side in RakPeer::verifyPreauthMac
// (processRbxOpenRequest2 calls it). A cross-version Player (0.573) and Studio
// host (0.574) derive different MACs, so the host rejects the Player with
// ID_INVALID_PASSWORD: in the Player log, "handshake failed ... version is out
// of date" + "Invalid password from <host>". It looks like a race because the
// MAC is time-keyed -- occasionally the windows line up and it connects.
//
// verifyPreauthMac returns a PreauthResult enum, and returns 1 ("allow") on its
// preauth-disabled early-out (`if (this->preauthEnabled == 0) return 1;`), which
// the caller treats as success. So forcing the whole function to `mov eax,1; ret`
// makes the host accept any client regardless of the MAC.
//
// The prologue shifts between builds, so we anchor on the version-stable preauth
// blob length check -- cmp rax,0x21 / cmp rax,0x32 (the 33- and 50-byte MAC
// formats) -- then walk back over the function body (which contains no 0xCC
// bytes) to the int3-padded `mov rax,rsp` entry. Confirmed on Studio 0.574;
// entry was 0x...ECFC10, ~0x55 bytes before the anchor.
// -----------------------------------------------------------------------------

static void PatchVerifyPreauthMac() {
    // Modern builds have a small preauth-enabled gate immediately before the
    // normal success return. NOP only its `jne` so the function keeps its own
    // ABI, unwind behavior, and return path.
    auto* modernGate = reinterpret_cast<uint8_t*>(ScanAny("verifyPreauthMac.modernGate", {
        "80 B9 50 0A 00 00 00 75 0A B8 01 00 00 00 E9 7C 06 00 00",
    }));
    if (modernGate) {
        uint8_t* branch = modernGate + 7;
        DWORD oldProtect = 0;
        if (!VirtualProtect(branch, 2, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            Log("preauth", "VirtualProtect failed @ %p", branch);
            return;
        }
        branch[0] = 0x90;
        branch[1] = 0x90;
        VirtualProtect(branch, 2, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), branch, 2);
        Log("preauth", "patched modern preauth gate @ %p", branch);
        return;
    }

    // Older builds do not have the modern gate. Retain the established fallback
    // for the 2023 --play path, guarded by its complete prologue and lencheck.
    // cmp rax,0x21 ; je ; cmp rax,0x32 ; je ; xor eax,eax
    auto* anchor = reinterpret_cast<uint8_t*>(ScanAny("verifyPreauthMac.lencheck", {
        "48 83 F8 21 74 ? 48 83 F8 32 74 ? 33 C0",
    }));
    if (!anchor) {
        Log("preauth", "length-check pattern not found -- host will reject cross-version clients");
        return;
    }

    // Walk back to the int3 padding that precedes the function. Older builds use
    // `mov rax,rsp`; modern builds use the longer nonvolatile-register prologue below.
    // Requiring a complete known prologue avoids treating an immediate 0xCC as
    // a function boundary.
    static const uint8_t kModernPrologue[] = {
        0x48, 0x89, 0x5C, 0x24, 0x18, 0x55, 0x56, 0x57,
        0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
        0x48, 0x8D, 0x6C, 0x24, 0xD0, 0x48, 0x81, 0xEC,
        0x30, 0x01, 0x00, 0x00,
    };
    uint8_t* entry = nullptr;
    for (int i = 1; i <= 0x140; i++) {
        if (anchor[-i] != 0xCC)
            continue;

        uint8_t* candidate = anchor - i + 1;
        const bool legacy = candidate[0] == 0x48 && candidate[1] == 0x8B && candidate[2] == 0xC4;
        const bool modern = std::memcmp(candidate, kModernPrologue, sizeof(kModernPrologue)) == 0;
        if (!legacy && !modern)
            continue;

        entry = candidate;
        break;
    }
    if (!entry) {
        Log("preauth", "could not find a known int3-padded entry near %p", anchor);
        return;
    }

    // mov eax,1 ; ret  -> PreauthResult "allow", returned before any stack setup.
    static const uint8_t kPatch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
    DWORD oldProtect;
    if (!VirtualProtect(entry, sizeof(kPatch), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("preauth", "VirtualProtect failed @ %p", entry);
        return;
    }
    std::memcpy(entry, kPatch, sizeof(kPatch));
    VirtualProtect(entry, sizeof(kPatch), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof(kPatch));
    Log("preauth", "patched verifyPreauthMac @ %p to always allow", entry);
}

// -----------------------------------------------------------------------------
// Patch: RakPeer "recently connected" 100ms window -> never reject
//
// THE actual intermittent team-test reject (verifyPreauthMac above turned out to
// be a red herring -- its result only selects a SessionCrypto log string and
// never rejects, which is why forcing it to 1 changed nothing). In
// RakPeer::AssignSystemAddressToRemoteSystemList (called from
// processRbxOpenRequest2 while handling OPEN_CONNECTION_REQUEST_2) a loop over the
// existing remotes does: if (NOW - remote[i].connectionTime) < 100ms, set a
// "recentlyConnected" flag; the caller then replies RbxOpenError reason 0x1A
// ("recently connected"), which the Player surfaces as the generic "handshake
// failed / version out of date / Invalid password". RakNet RETRANSMITS the open
// request during the slower cross-version handshake, so retransmit #2 lands inside
// the 100ms window opened by #1 and trips this -- a wall-clock timer is the only
// time-based gate, which is exactly why it's intermittent (the first packet of a
// burst can win, so the join sometimes succeeds).
//
// Fix: NOP the `jb set-recentlyConnected` so the flag is never set; the loop just
// continues and the connection is added normally. Anchor on the version-stable
// elapsed/100ms compare `sub rcx,rdx ; cmp rcx,0x64 ; jb` -- the 0x64 (=100ms)
// constant survives across builds even though the connectionTime struct offset
// (0x1188 in 0.548, 0x10B8 in 0.574) and the prologue do not. Confirmed on Studio
// 0.574: jb @ 0x...EB9F6B (72 0A) -> set-flag.
// -----------------------------------------------------------------------------

static void PatchRecentlyConnectedWindow() {
    // sub rcx,rdx ; cmp rcx,0x64 ; jb <set recentlyConnected=1>
    // 0.574 uses a short branch; newer builds use the near form.
    auto* anchor = reinterpret_cast<uint8_t*>(ScanAny("recentlyConnected.window", {
        "48 2B CA 48 83 F9 64 72 ?",
        "48 2B CA 48 83 F9 64 0F 82 ? ? ? ?",
    }));
    if (!anchor) {
        Log("recent", "100ms-window pattern not found -- host may still reject handshake retransmits");
        return;
    }
    uint8_t* jb = anchor + 7;
    size_t branchSize = 0;
    if (jb[0] == 0x72)
        branchSize = 2;
    else if (jb[0] == 0x0F && jb[1] == 0x82)
        branchSize = 6;
    else {
        Log("recent", "expected short/near jb at %p, got %02x %02x -- not patching",
            jb, jb[0], jb[1]);
        return;
    }
    // Remove the complete conditional branch: the elapsed<100ms case no longer
    // sets recentlyConnected, so the duplicate-suppression reject never fires.
    DWORD oldProtect;
    if (!VirtualProtect(jb, branchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("recent", "VirtualProtect failed @ %p", jb);
        return;
    }
    std::memset(jb, 0x90, branchSize);
    VirtualProtect(jb, branchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), jb, branchSize);
    Log("recent", "patched recently-connected 100ms reject @ %p (NOP jb) -- retransmit race killed", jb);
}

// -----------------------------------------------------------------------------
// Patch: RakPeer::ParseConnectionRequestPacket password compare -> always accept
//
// THE gate that emits the "Invalid password" the Player actually sees (verified:
// the reject block writes `mov byte[rbp+0x250],0x18` = ID_INVALID_PASSWORD). After
// the crypto open-connection handshake, the host parses the online
// ID_CONNECTION_REQUEST and compares the client's password to the one it stored via
// SetIncomingPassword: length at this+0x198, bytes at this+0x199 (0.574 offsets;
// 0x188/0x189 in 0.548). Cross-version (0.573 Player vs 0.574 host) the length
// and/or bytes don't match, so the host rejects with ID_INVALID_PASSWORD ->
// "handshake failed / version out of date / Invalid password".
//
//   movzx eax,[rsi+0x198]   ; stored pw length
//   cmp   eax,ebx           ; vs client pw length
//   jne   reject            ; (1) NOP
//   mov   r8d,eax ; lea rdx,[rsi+0x199] ; call memcmp ; test eax,eax
//   jne   reject            ; (2) NOP
//   mov   dword[rdi+0x1120],5  ; accept (connectMode=5)
//
// NOP both `jne reject` so any password falls through to accept. Anchor on the
// register-stable `cmp eax,ebx ; jne ; mov r8d,eax ; lea rdx,[rsi+disp32]` core
// (unique in the image; the struct offsets and call rel vary by build, the
// instruction *lengths* don't, so jne2 is a fixed +21 from the anchor).
// -----------------------------------------------------------------------------

static void PatchConnectionRequestPassword() {
    auto* anchor = reinterpret_cast<uint8_t*>(ScanAny("connReqPassword.compare", {
        "3B C3 75 ? 44 8B C0 48 8D 96",
    }));
    if (!anchor) {
        Log("pwgate", "ParseConnectionRequest password-compare pattern not found -- host will still reject with ID_INVALID_PASSWORD");
        return;
    }
    // jne1 (length mismatch) right after `cmp eax,ebx`; jne2 (memcmp mismatch) after
    // lea(7)+call(5)+test(2) => +21. Verify the test/jne shape before touching anything.
    uint8_t* jne1 = anchor + 2;
    uint8_t* jne2 = anchor + 21;
    if (jne1[0] != 0x75 || anchor[19] != 0x85 || anchor[20] != 0xC0 || jne2[0] != 0x75) {
        Log("pwgate", "unexpected layout @ %p (j1=%02x test=%02x%02x j2=%02x) -- not patching",
            anchor, jne1[0], anchor[19], anchor[20], jne2[0]);
        return;
    }
    DWORD oldProtect;
    if (!VirtualProtect(anchor, 32, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("pwgate", "VirtualProtect failed @ %p", anchor);
        return;
    }
    jne1[0] = 0x90; jne1[1] = 0x90;   // 75 xx -> 90 90
    jne2[0] = 0x90; jne2[1] = 0x90;
    VirtualProtect(anchor, 32, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), anchor, 32);
    Log("pwgate", "patched ParseConnectionRequest password compare @ %p (NOP'd both jne) -- ID_INVALID_PASSWORD killed", anchor);
}

// -----------------------------------------------------------------------------
// Pattern scanning
// -----------------------------------------------------------------------------

// Try patterns in order; first one that produces exactly one match wins.
// Returns nullptr if every alternative misses (caller logs and bails).
void* LocalRccShared::ScanAny(
    const char* label, std::initializer_list<const char*> patterns) {
    int idx = 0;
    for (const char* sig : patterns) {
        auto p = hook::pattern(sig);
        const size_t count = p.size();
        if (count == 1) {
            void* addr = p.get(0).get<void>(0);
            Log("scan", "%s matched alt #%d @ %p", label, idx, addr);
            return addr;
        }
        if (count > 1)
            Log("scan", "%s alt #%d is ambiguous (%zu matches) -- ignoring", label, idx, count);
        idx++;
    }
    Log("scan", "%s: no pattern matched (tried %d)", label, idx);
    return nullptr;
}

static bool ScanForAddresses() {
#if defined(_WIN64) && defined(_M_X64)
    // Target: Roblox Studio 0.574 (May 2023). Patterns lifted from the
    // local_rcc fork's 2023M / 2023 modes, with the original "latest" patterns
    // kept as fallbacks so the same DLL still loads on adjacent builds.
    //
    // CRITICAL: Hooking.Patterns wildcards are SINGLE `?` (one char = one
    // wildcard byte). Two `??` parse as TWO wildcards and silently mis-match.

    // ---- LuaVM::compile -----------------------------------------------------
    // 2023M (May-Jun 2023) adds a trailing C3 (ret) right after the prologue;
    // older Studios have the same prologue without the ret. The C3 disambiguates
    // from another function with the identical 22-byte prologue (verified: plain
    // pattern hits twice in 0.574, with-C3 hits once at the right address).
    void* compileAddr = ScanAny("compile", {
        "33 C0 48 C7 41 18 0F 00 00 00 48 89 01 48 89 41 10 88 01 48 8B C1 C3",  // 2023M
        "33 C0 48 C7 41 18 0F 00 00 00 48 89 01 48 89 41 10 88 01 48 8B C1",     // pre-2023M (ambiguous)
    });
    if (!compileAddr) return false;
    gCompile = reinterpret_cast<types::compile_fn>(compileAddr);

    // ---- Replicator::deserializeItem outer/inner pair ----------------------
    // These are separate dispatchers. In modern builds, ClientQoSItem is a concrete
    // outer case while the inner dispatcher rejects it. Hooking only inner and
    // returning early leaves the bitstream cursor in the QoS payload, which the
    // next read mistakes for a class network type (disconnect 260).
    void* deserOuterAddr = ScanAny("deserializeItem.outer", {
        "48 89 5C 24 08 48 89 74 24 18 48 89 54 24 10 55 57 41 54 41 56 41 57 48 8D 6C 24 D0 48 81 EC 30 01 00 00 45 8B F9 49 8B F0 48 8B FA 4C 8B F1 45 33 E4",  // 0.574
        "48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC C0 00 00 00 45 8B F9 4D 8B F0 48 8B FA 48 8B F1 45 33 E4",       // 0.712
        "48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC B0 00 00 00 45 8B F9 4D 8B F0 48 8B FA 48 8B F1 45 33 E4",       // modern fallback
    });
    if (!deserOuterAddr) return false;
    gDeserializeItemOuter = reinterpret_cast<types::deserialize_item_fn>(deserOuterAddr);

    void* deserInnerAddr = ScanAny("deserializeItem.inner", {
        "48 89 5C 24 08 48 89 54 24 10 55 56 57 41 56 41 57 48 8D 6C 24 C9 48 81 EC C0 00 00 00 4D 8B F0 48 8B F2 48 8B D1 33 DB",                              // 0.574
        "48 89 5C 24 ? 48 89 74 24 ? 48 89 54 24 ? 55 57 41 56 48 8B EC 48 83 EC 40 49 8B F8",                                                                              // adjacent 2023 builds
        "48 89 5C 24 08 48 89 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC D0 00 00 00 45 8B F9 4D 8B F0 48 8B FA 48 8B F1 45 33 E4",       // 0.719 candidate
    });
    if (!deserInnerAddr) return false;
    gDeserializeItemInner = reinterpret_cast<types::deserialize_item_fn>(deserInnerAddr);

    // ---- NetworkSchema::generateSchemaDefinitionPacket ---------------------
    // We patch the immediate at +4 (the `06` byte in `mov byte ptr [rsp+disp], 6`).
    // 2023 ends in `48 3B 15` (cmp r/m64, [rip+disp32]); 2022 ends in `48 8D 05` (lea).
    void* schemaAddr = ScanAny("typeForProperty", {
        "C6 44 24 ? 06 EB ? 48 3B 15",  // 2023 (matches 0.574)
        "C6 44 24 ? 06 EB ? 48 8D 05",  // 2022
    });
    if (!schemaAddr) return false;
    gTypeForPropertyImm = reinterpret_cast<uintptr_t>(schemaAddr) + 4;
    Log("scan", "typeForProperty imm @ %p", (void*)gTypeForPropertyImm);

    return true;
#else
    Log("scan", "LocalRcc is x64-only");
    return false;
#endif
}

// -----------------------------------------------------------------------------
// DLL entrypoint
// -----------------------------------------------------------------------------

// Read by both code paths below. noobhook (via the Injector) sets NOOBHOOK_SIDE to
// the launch role: "client", "server", or "studio". Only "server" means this Studio
// is acting as a team-test host, so only then do we install the rsblox/local_rcc
// replication machinery. "studio" (regular Play Solo) and anything else -- including
// the variable being unset -- means vanilla script handling. Defaulting to the safe
// (solo) path is deliberate: an old or side-less injector, or LocalRcc loaded by
// hand, all stay out of the way of in-process script execution.
static bool IsTeamTestMode() {
    char buf[16] = {0};
    DWORD n = GetEnvironmentVariableA("NOOBHOOK_SIDE", buf, sizeof(buf));
    return n > 0 && n < sizeof(buf) && _stricmp(buf, "server") == 0;
}

static DWORD InitializeLocalRcc() {
    gLog = freopen("noobhook_x86-64_localrcc.log", "w", stderr);
    Log("main", "LocalRcc starting (rsblox/local_rcc port, Luau %s)",
        NOOBWARRIOR_LOCALRCC_LUAU_VERSION);

    const StudioImageIdentity identity = GetStudioImageIdentity();
    if (identity.valid) {
        const char* exactLabel = IsExactStudio719(identity) ? " (exact 0.719.0.7191339)" :
            (IsExactStudio574(identity) ? " (exact 0.574.0.5740446)" : "");
        Log("main", "Studio image fingerprint: timestamp=0x%08lx size=0x%08lx%s",
            identity.timeDateStamp, identity.sizeOfImage,
            exactLabel);
    }

    // Bytecode version alone cannot distinguish adjacent modern Luau tags.
    // Match the compiler tag to exact Studio builds as
    // well, otherwise a stale CMake cache can create valid-looking but
    // incompatible bytecode and Player only reports "bytecode corrupted".
    if (IsExactStudio719(identity) &&
        std::strcmp(NOOBWARRIOR_LOCALRCC_LUAU_VERSION, "0.719") != 0) {
        Log("main", "Studio 0.719 requires the Luau 0.719 sidecar (built with %s)",
            NOOBWARRIOR_LOCALRCC_LUAU_VERSION);
        return ERROR_REVISION_MISMATCH;
    }
    if (std::strcmp(NOOBWARRIOR_LOCALRCC_LUAU_VERSION, "0.719") == 0 &&
        !IsExactStudio719(identity)) {
        Log("main", "Luau 0.719 sidecar refuses an unknown Studio fingerprint");
        return ERROR_REVISION_MISMATCH;
    }
    if (IsExactStudio574(identity) &&
        std::strcmp(NOOBWARRIOR_LOCALRCC_LUAU_VERSION, "0.574") != 0) {
        Log("main", "Studio 0.574 requires the Luau 0.574 sidecar (built with %s)",
            NOOBWARRIOR_LOCALRCC_LUAU_VERSION);
        return ERROR_REVISION_MISMATCH;
    }
    if (std::strcmp(NOOBWARRIOR_LOCALRCC_LUAU_VERSION, "0.574") == 0 &&
        !IsExactStudio574(identity)) {
        Log("main", "Luau 0.574 sidecar refuses an unknown Studio fingerprint");
        return ERROR_REVISION_MISMATCH;
    }

    if (!ResolveAllocate() || !ResolveDeallocate()) {
        Log("main", "rbxAllocate/rbxDeallocate not exported -- aborting (not Studio?)");
        return 1;
    }

    const MH_STATUS initializeStatus = MH_Initialize();
    if (initializeStatus != MH_OK && initializeStatus != MH_ERROR_ALREADY_INITIALIZED) {
        Log("main", "MH_Initialize failed: %d", static_cast<int>(initializeStatus));
        return 1;
    }

    const bool teamTest = IsTeamTestMode();
    Log("main", "Launch mode: %s", teamTest
        ? "teamtest -- local_rcc replication ON"
        : "solo -- vanilla script handling (connection hooks only)");

    if (teamTest) {
        // These are independent Studio-server compatibility patches. Apply them
        // before the heavier local_rcc pattern scans and MinHook setup so an
        // unrelated compile/deserialize signature miss cannot leave RakNet's
        // incoming-password gate active.
        PatchConnectionRequestPassword();
        PatchModernKeyExchange();
        PatchVerifyPreauthMac();
        PatchRecentlyConnectedWindow();

        // Exact 0.719 Studio leaves the hidden VoiceChatService RCC gates false
        // in its normal Team Test launch settings. This bridge fills those two
        // settings from Core's per-universe policy before ConfigureWithoutRun
        // consumes them. Other Studio fingerprints remain untouched.
#if defined(NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC)
        if (IsExactStudio719(identity))
            VoiceChat719::InstallHooks();
#endif
    }

    // Studio-offline web-stack hooks are needed in BOTH modes -- they're what let
    // Studio reach the local emulator at all. Install them first, unconditionally.
    // If none of the three patterns match, log and keep going.
    if (!InstallOfflineHooks())
        Log("main", "No studio-offline patterns matched (connection bypass disabled)");

    // WebView2 login also needs StudioCookieManager to accept the security cookie
    // offline. Raw byte patch (independent of MinHook), also needed in both modes.
    PatchSecurityCookie();

    // External clients do not compile replicated Source. Studio's LuaVM::compile
    // implementation is an empty-string stub even when Studio and Player are the
    // exact same build, so every team-test profile needs the pinned Luau compiler.
    // Force ProtectedStringBytecode as well so the populated legalScripts bytecode
    // is selected instead of source replication. Older --play behavior is unchanged
    // because this machinery is still installed only for the server role.
    TypeForPropertyPatchState schemaPatch;
    if (teamTest) {
        if (!ScanForAddresses()) {
            Log("main", "Pattern scan failed -- LocalRcc must be updated for this Studio build");
            return 1;
        }

        MH_STATUS st = MH_OK;
        st = MH_CreateHook(reinterpret_cast<LPVOID>(gCompile),
                           reinterpret_cast<LPVOID>(&CompileHook),
                           reinterpret_cast<LPVOID*>(&gOrigCompile));
        if (st != MH_OK) {
            Log("main", "Hook(compile) failed: %d", (int)st);
            RemoveTeamTestHooks();
            return 1;
        }

        st = MH_CreateHook(reinterpret_cast<LPVOID>(gDeserializeItemOuter),
                           reinterpret_cast<LPVOID>(&DeserializeItemOuterHook),
                           reinterpret_cast<LPVOID*>(&gOrigDeserializeOuter));
        if (st != MH_OK) {
            Log("main", "Hook(deserializeItem.outer) failed: %d", (int)st);
            RemoveTeamTestHooks();
            return 1;
        }

        st = MH_CreateHook(reinterpret_cast<LPVOID>(gDeserializeItemInner),
                           reinterpret_cast<LPVOID>(&DeserializeItemInnerHook),
                           reinterpret_cast<LPVOID*>(&gOrigDeserializeInner));
        if (st != MH_OK) {
            Log("main", "Hook(deserializeItem.inner) failed: %d", (int)st);
            RemoveTeamTestHooks();
            return 1;
        }

        if (!PatchTypeForProperty(schemaPatch)) {
            Log("main", "Could not enable ProtectedStringBytecode replication");
            RemoveTeamTestHooks();
            return 1;
        }
    }

    MH_STATUS est = MH_EnableHook(MH_ALL_HOOKS);
    if (est != MH_OK) {
        Log("main", "EnableHook failed: %d", (int)est);
        if (teamTest) {
            // MH_EnableHook(MH_ALL_HOOKS) may have enabled a prefix before
            // reporting failure. Disable globally, then explicitly remove the
            // team-test hooks and restore the schema byte.
            const MH_STATUS disableStatus = MH_DisableHook(MH_ALL_HOOKS);
            if (disableStatus != MH_OK && disableStatus != MH_ERROR_DISABLED)
                Log("main", "DisableHook(all) during rollback failed: %d", (int)disableStatus);
            RemoveTeamTestHooks();
            if (!RestoreTypeForProperty(schemaPatch))
                Log("main", "CRITICAL: could not restore typeForProperty after hook failure");
        }
        return 1;
    }
    Log("main", "Enabled hooks (offline%s)",
        teamTest ? " + compile + deserializeItem outer/inner" : " only");

    if (teamTest)
        Log("main", "Patched RBX::Network::NetworkSchema::typeForProperty");

    Log("main", "LocalRcc ready - credits: 7ap & Epix @ https://github.com/rsblox");
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI NoobLocalRccInitialize(LPVOID moduleBase) {
    if (moduleBase) {
        InterlockedCompareExchangePointer(
            reinterpret_cast<PVOID volatile*>(&gLocalRccModule), moduleBase, nullptr);
    }

    LONG previous = InterlockedCompareExchange(&gInitializationState, 1, 0);
    if (previous == 0) {
        const DWORD result = gLocalRccModule ? InitializeLocalRcc() : ERROR_INVALID_HANDLE;
        InterlockedExchange(&gInitializationResult, static_cast<LONG>(result));
        InterlockedExchange(&gInitializationState, result == ERROR_SUCCESS ? 2 : 3);
        return result;
    }
    if (previous == 2)
        return ERROR_SUCCESS;
    if (previous == 3)
        return static_cast<DWORD>(InterlockedCompareExchange(&gInitializationResult, 0, 0));

    const ULONGLONG deadline = GetTickCount64() + 120000;
    while (GetTickCount64() < deadline) {
        const LONG state = InterlockedCompareExchange(&gInitializationState, 0, 0);
        if (state == 2)
            return ERROR_SUCCESS;
        if (state == 3)
            return static_cast<DWORD>(InterlockedCompareExchange(&gInitializationResult, 0, 0));
        Sleep(1);
    }
    return ERROR_TIMEOUT;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gLocalRccModule = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
