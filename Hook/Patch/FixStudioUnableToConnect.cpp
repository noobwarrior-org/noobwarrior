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
 // File: FixStudioUnableToConnect.cpp
 // Started by: Hattozo
 // Started on: 5/19/2026
 // Description: Fixes "Studio is unable to connect. Please check your internet connection and try again." error
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::FixStudioUnableToConnect() {
    // NOP the call to getDiscoveryConfiguration (Studio 0.574 x64).
    // That function throws when the OAuth metadata fetch fails, causing the
    // "Studio is unable to connect" dialog. The success path after the call
    // reads [rsp+208h], which is initialized before this call site (confirmed
    // by an earlier shortcut EB 4D that also bypasses the call and reaches the
    // same success continuation). NOP-ing the call makes execution fall through
    // to the existing 90 EB 09 bytes, which jump directly to that success path.
    /*auto pattern = hook::pattern("E8 12 7F 29 01 90 EB 09 33 F6");
    if (!pattern.count_hint(1).empty()) {
        Out("FixStudioUnableToConnect", "Patching getDiscoveryConfiguration call site");
        const uint8_t bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(pattern.get(0).get<uint8_t>(0)), bytes, sizeof(bytes));
    }*/

    // Notes for future work — the dialog is triggered by AuthTokenManager::fetchMetadata()
    // failing with TlsVerificationFail when requesting
    // https://apis.roblox.com/oauth/.well-known/openid-configuration. The Hook redirects
    // this to 127.0.0.1:53640 where the emulator serves the discovery JSON, but libcurl
    // rejects the emulator's self-signed cert (CN=localhost; cert isn't a trusted root;
    // hostname doesn't match apis.roblox.com). The 169 conditional VERIFYPEER wrapper
    // patches don't cover this call site because fetchMetadata never calls setopt for
    // VERIFYPEER / VERIFYHOST — it relies on libcurl's defaults (VERIFYPEER=1, VERIFYHOST=2).
    //
    // Possible fixes (any one of):
    //  - Install noobWarrior's cert.pem as a Trusted Root (Cert:\LocalMachine\Root) AND
    //    regenerate cert.pem with CN=apis.roblox.com + SAN for *.roblox.com so VERIFYHOST
    //    also passes.
    //  - Find X509_verify_cert (statically linked OpenSSL) and patch its prologue to
    //    `mov eax, 1; ret` to force all certificate verifications to succeed.
    //  - Find AuthTokenManager::fetchMetadata in .text near VA 0x1408BA000-0x1408BAB00
    //    (4 LEA refs to "AuthTokenManagerFetchMetadata") and patch its prologue to fake
    //    a successful fetch with hardcoded metadata pointing at the local emulator.
    //  - MinHook curl_easy_setopt inside Studio to clamp VERIFYPEER / VERIFYHOST to 0.
    // Picking the right approach needs a live debugger (x64dbg) — static analysis alone
    // can't reliably locate fetchMetadata's prologue or the OpenSSL helpers in the
    // 60MB stripped binary.

    /*

    auto pattern2 = hook::pattern("E9 C3 8D 16 00");
    if (!pattern2.count_hint(1).empty()) {
        Out("FixStudioUnableToConnect", "Found pattern 2!");
        uintptr_t* address = pattern2.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0xC3, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }*/

    /*auto pattern = hook::pattern("E8 B4 03 00 00 CC CC CC CC");
    if (!pattern.count_hint(1).empty()) {
        Out("FixStudioUnableToConnect", "Found pattern!");
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }*/

    /*auto pattern2 = hook::pattern("E8 E8 03 00 00 CC CC CC CC");
    if (!pattern2.count_hint(1).empty()) {
        Out("FixStudioUnableToConnect", "Found pattern 2!");
        uintptr_t* address = pattern2.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }*/

    /*auto pattern = hook::pattern("0F 84 AE 04 00 00 8B 40 04 85 C0 0F 84 A3 04 00 00 48 83 7B 38 00 0F 84 98 04 00 00");
    if (!pattern.count_hint(1).empty()) {
        Out("FixStudioUnableToConnect", "Found pattern!");
        uintptr_t* address = pattern.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = {
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // NOP first je
            0x90, 0x90, 0x90,                   // NOP mov eax,[rax+4]
            0x90, 0x90,                         // NOP test eax,eax
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // NOP second je
            0x90, 0x90, 0x90, 0x90, 0x90,       // NOP cmp [rbx+38],0
            0x90, 0x90, 0x90, 0x90, 0x90, 0x90  // NOP third je
        };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // Patch the authenticateOnStartup function to return immediately
    // Function starts at 7FF6B3B50370
    // Replace first bytes with: xor eax,eax; ret (return 0)
    auto loginPatch = hook::pattern("48 89 74 24 18 55 57 41 56 48 8D 6C 24 B9 48 81 EC 90 00 00 00 48 8B F9 E8");
    if (!loginPatch.count_hint(1).empty()) {
        Out("FixLogin", "Found authenticateOnStartup!");
        uintptr_t* address = loginPatch.get(0).get<uintptr_t>(0);
        // xor eax, eax; ret
        const uint8_t bytes[] = { 0x31, 0xC0, 0xC3 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
    */

    /*
    auto pattern2 = hook::pattern("E8 0E 95 05 00 90 48 8B 55 3F");
    if (!pattern2.count_hint(1).empty()) {
        uintptr_t* address = pattern2.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    auto pattern3 = hook::pattern("41 83 FE 01 75 7B 0F 10 43 08");
    if (!pattern3.count_hint(1).empty()) {
        Out("FixStudioAuth2", "Found login branch pattern!");
        uintptr_t* address = pattern3.get(0).get<uintptr_t>(0);
        // Change jne (+7B) to jmp always, skipping the auth block
        const uint8_t bytes[] = { 0x41, 0x83, 0xFE, 0x01, 0xEB, 0x7B };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // jne -> jmp at 7FF6B3BA9BF0
    // 75 64 -> EB 64
    auto pattern4 = hook::pattern("48 85 DB 75 64 48 8D 0D 4F 51 88 03");
    if (!pattern4.count_hint(1).empty()) {
        Out("FixStudioCookie", "Found cookie check pattern!");
        uintptr_t* address = pattern4.get(0).get<uintptr_t>(0);
        // Skip past test rbx,rbx (3 bytes) to the jne
        const uint8_t bytes[] = { 0x48, 0x85, 0xDB, 0xEB, 0x64 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // jne -> jmp at 7FF6B3BA9C62
    auto pattern5 = hook::pattern("84 C0 75 62 48 8D 0D ED 50 88 03");
    if (!pattern5.count_hint(1).empty()) {
        Out("FixStudioCookie", "Found refresh token check pattern!");
        uintptr_t* address = pattern5.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x84, 0xC0, 0xEB, 0x62 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // je -> nop nop at 7FF6B3BA9FCE (skips success if flag=0)
    auto patternAuth1 = hook::pattern("0F 84 7C 00 00 00 49 8B 4E 78");
    if (!patternAuth1.count_hint(1).empty()) {
        uintptr_t* address = patternAuth1.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }

    // je -> nop nop at 7FF6B3BA9FDF (skips success if check fails)
    auto patternAuth2 = hook::pattern("74 6F 48 8D 0D A8 4D 88 03");
    if (!patternAuth2.count_hint(1).empty()) {
        uintptr_t* address = patternAuth2.get(0).get<uintptr_t>(0);
        const uint8_t bytes[] = { 0x90, 0x90 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(address), bytes, sizeof(bytes));
    }
    */
}
