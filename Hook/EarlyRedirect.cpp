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
// File: EarlyRedirect.cpp
// Started by: Hattozo
// Started on: 8/11/2026
// Description: Loader-free Winsock provider redirect installed before Player startup.
// Slopped by OpenAI Codex

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

extern "C" uint16_t NoobEarlyByteSwap16(uint16_t value);

namespace {

using Socket = uintptr_t;
using ConnectFn = int (WINAPI*)(Socket, const void*, int);
using WsaConnectFn = int (WINAPI*)(Socket, const void*, int, void*, void*, void*, void*);
using WspConnectFn = int (WINAPI*)(Socket, const void*, int, void*, void*, void*, void*, int*);
using GetProcAddressFn = FARPROC (WINAPI*)(HMODULE, LPCSTR);

struct Ipv4Address {
    uint16_t family;
    uint16_t port;
    uint8_t address[4];
    uint8_t padding[8];
};

constexpr uint16_t kAddressFamilyIpv4 = 2;

static bool IsConnectName(const char* name) {
    return name != nullptr &&
        name[0] == 'c' && name[1] == 'o' && name[2] == 'n' && name[3] == 'n' &&
        name[4] == 'e' && name[5] == 'c' && name[6] == 't' && name[7] == '\0';
}

static bool IsWsaConnectName(const char* name) {
    return name != nullptr &&
        name[0] == 'W' && name[1] == 'S' && name[2] == 'A' && name[3] == 'C' &&
        name[4] == 'o' && name[5] == 'n' && name[6] == 'n' && name[7] == 'e' &&
        name[8] == 'c' && name[9] == 't' && name[10] == '\0';
}

static bool BuildRedirectedAddress(const void* source, int sourceLength, Ipv4Address& redirected);

} // namespace

extern "C" {

__declspec(dllexport) uintptr_t NoobEarlyOriginalGetProcAddress = 0;
__declspec(dllexport) uintptr_t NoobEarlyOriginalConnect = 0;
__declspec(dllexport) uintptr_t NoobEarlyOriginalWsaConnect = 0;
__declspec(dllexport) uintptr_t NoobEarlyOriginalWspConnect = 0;
__declspec(dllexport) uint16_t NoobEarlyHttpPort = 8080;
__declspec(dllexport) uint16_t NoobEarlyHttpsPort = 53640;
__declspec(dllexport) volatile uint64_t NoobEarlyGetProcAddressCalls = 0;
__declspec(dllexport) volatile uint64_t NoobEarlyConnectResolutions = 0;
__declspec(dllexport) volatile uint64_t NoobEarlyWsaConnectResolutions = 0;
__declspec(dllexport) volatile uint64_t NoobEarlySocketCalls = 0;
__declspec(dllexport) volatile uint64_t NoobEarlyWspConnectCalls = 0;
__declspec(dllexport) volatile uint64_t NoobEarlyRedirectedSocketCalls = 0;

__declspec(dllexport) int WINAPI NoobEarlyConnect(Socket socket, const void* address, int addressLength) {
    ++NoobEarlySocketCalls;
    const auto original = reinterpret_cast<ConnectFn>(NoobEarlyOriginalConnect);
    if (original == nullptr)
        return -1;

    Ipv4Address redirected = {};
    if (BuildRedirectedAddress(address, addressLength, redirected)) {
        ++NoobEarlyRedirectedSocketCalls;
        return original(socket, &redirected, sizeof(redirected));
    }
    return original(socket, address, addressLength);
}

__declspec(dllexport) int WINAPI NoobEarlyWsaConnect(Socket socket, const void* address,
                                                     int addressLength, void* callerData,
                                                     void* calleeData, void* sendQuality,
                                                     void* receiveQuality) {
    ++NoobEarlySocketCalls;
    const auto original = reinterpret_cast<WsaConnectFn>(NoobEarlyOriginalWsaConnect);
    if (original == nullptr)
        return -1;

    Ipv4Address redirected = {};
    if (BuildRedirectedAddress(address, addressLength, redirected)) {
        ++NoobEarlyRedirectedSocketCalls;
        return original(socket, &redirected, sizeof(redirected), callerData, calleeData,
                        sendQuality, receiveQuality);
    }
    return original(socket, address, addressLength, callerData, calleeData,
                    sendQuality, receiveQuality);
}

// WS2_32 reaches the service provider through this eight-argument SPI entry. Keeping the
// redirect here lets the injector replace a writable provider-table pointer without touching
// Roblox, Hyperion, or a Windows image/code page.
__declspec(dllexport) int WINAPI NoobEarlyWspConnect(Socket socket, const void* address,
                                                     int addressLength, void* callerData,
                                                     void* calleeData, void* sendQuality,
                                                     void* receiveQuality, int* error) {
    ++NoobEarlySocketCalls;
    ++NoobEarlyWspConnectCalls;
    const auto original = reinterpret_cast<WspConnectFn>(NoobEarlyOriginalWspConnect);
    if (original == nullptr)
        return -1;

    Ipv4Address redirected = {};
    if (BuildRedirectedAddress(address, addressLength, redirected)) {
        ++NoobEarlyRedirectedSocketCalls;
        return original(socket, &redirected, sizeof(redirected), callerData, calleeData,
                        sendQuality, receiveQuality, error);
    }
    return original(socket, address, addressLength, callerData, calleeData,
                    sendQuality, receiveQuality, error);
}

__declspec(dllexport) FARPROC WINAPI NoobEarlyGetProcAddress(HMODULE module, LPCSTR procName) {
    ++NoobEarlyGetProcAddressCalls;
    const auto original = reinterpret_cast<GetProcAddressFn>(NoobEarlyOriginalGetProcAddress);
    if (original == nullptr)
        return nullptr;

    FARPROC resolved = original(module, procName);
    if (resolved == nullptr || procName == nullptr || IS_INTRESOURCE(procName))
        return resolved;

    if (IsConnectName(procName)) {
        ++NoobEarlyConnectResolutions;
        NoobEarlyOriginalConnect = reinterpret_cast<uintptr_t>(resolved);
        return reinterpret_cast<FARPROC>(&NoobEarlyConnect);
    }
    if (IsWsaConnectName(procName)) {
        ++NoobEarlyWsaConnectResolutions;
        NoobEarlyOriginalWsaConnect = reinterpret_cast<uintptr_t>(resolved);
        return reinterpret_cast<FARPROC>(&NoobEarlyWsaConnect);
    }
    return resolved;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) {
    return TRUE;
}

} // extern "C"

namespace {

static bool BuildRedirectedAddress(const void* source, int sourceLength, Ipv4Address& redirected) {
    if (source == nullptr || sourceLength < static_cast<int>(sizeof(Ipv4Address)))
        return false;
    const auto* input = static_cast<const Ipv4Address*>(source);
    if (input->family != kAddressFamilyIpv4)
        return false;

    const uint16_t originalPort = NoobEarlyByteSwap16(input->port);
    uint16_t emulatorPort = 0;
    if (originalPort == 80)
        emulatorPort = NoobEarlyHttpPort;
    else if (originalPort == 443)
        emulatorPort = NoobEarlyHttpsPort;
    else
        return false;

    redirected = *input;
    redirected.port = NoobEarlyByteSwap16(emulatorPort);
    redirected.address[0] = 127;
    redirected.address[1] = 0;
    redirected.address[2] = 0;
    redirected.address[3] = 1;
    return true;
}

} // namespace
