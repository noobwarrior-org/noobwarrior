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
// File: RemoveTLSVerification.cpp
// Started by: Hattozo
// Started on: 3/22/2026
// Description: Thanks to VisualPlugin for making this guide
// https://github.com/Windows81/Roblox-Freedom-Distribution-Guides/blob/main/PatchTLSVerification/README.md
#include "Patches.h"
#include <windows.h>

void NoobHook::Patches::RemoveTLSVerification() {
    auto x64VerifyPeer = hook::pattern("41 B8 01 00 00 00 BA 40 00 00 00");
    if (x64VerifyPeer.size() > 0) {
        Out("RemoveTLSVerification", "Patching x64 CURLOPT_SSL_VERIFYPEER");
        x64VerifyPeer.for_each_result([](hook::pattern_match match) {
            const uint8_t patch[] = { 0x00 };
            NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(match.get<uint8_t>(2)), patch, sizeof(patch));
        });
    } else {
        auto x86VerifyPeer = hook::pattern("6A 01 6A 40 FF B7 48 01 00 00");
        if (!x86VerifyPeer.count_hint(1).empty()) {
            Out("RemoveTLSVerification", "Patching x86 CURLOPT_SSL_VERIFYPEER");
            const uint8_t patch[] = { 0x00 };
            NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(x86VerifyPeer.get(0).get<uint8_t>(1)), patch, sizeof(patch));
        }
    }

    // The VerifyHost guard uses REX.B (41) + MOD/RM 86 (R14-relative), not F6 85 (RBP-relative).
    // Old pattern "F6 85 50 0A 00 00 02 74 1B BA 51 00 00 00" never matched; correct it here.
    auto x64VerifyHostGuard = hook::pattern("41 F6 86 50 0A 00 00 02 74 1B BA 51 00 00 00");
    if (!x64VerifyHostGuard.count_hint(1).empty()) {
        Out("RemoveTLSVerification", "Patching x64 CURLOPT_SSL_VERIFYHOST guard (je->jmp)");
        const uint8_t patch[] = { 0xEB };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(x64VerifyHostGuard.get(0).get<uint8_t>(8)), patch, sizeof(patch));
    }

    auto x86VerifyHost = hook::pattern("6A 02 6A 51 FF B7 48 01 00 00");
    if (!x86VerifyHost.count_hint(1).empty()) {
        Out("RemoveTLSVerification", "Patching x86 CURLOPT_SSL_VERIFYHOST");
        const uint8_t patch[] = { 0x00 };
        NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(x86VerifyHost.get(0).get<uint8_t>(1)), patch, sizeof(patch));
    }

    // VERIFYPEER: pattern `mov edx, 0x40; lea r8d, [rdx-0x3F]` -> patch the LEA
    auto verifyPeerSetopt = hook::pattern("BA 40 00 00 00 44 8D 42 C1");
    if (verifyPeerSetopt.size() > 0) {
        Out("RemoveTLSVerification", "Patching %d VERIFYPEER setopt sites (lea->xor)", verifyPeerSetopt.size());
        verifyPeerSetopt.for_each_result([](hook::pattern_match match) {
            const uint8_t p[] = { 0x45, 0x31, 0xC0, 0x90 };
            NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(match.get<uint8_t>(5)), p, sizeof(p));
        });
    }

    // VERIFYHOST: pattern `mov edx, 0x51; lea r8d, [rdx-0x4F]` -> patch the LEA
    auto verifyHostSetopt = hook::pattern("BA 51 00 00 00 44 8D 42 B1");
    if (verifyHostSetopt.size() > 0) {
        Out("RemoveTLSVerification", "Patching %d VERIFYHOST setopt sites (lea->xor)", verifyHostSetopt.size());
        verifyHostSetopt.for_each_result([](hook::pattern_match match) {
            const uint8_t p[] = { 0x45, 0x31, 0xC0, 0x90 };
            NoobHook::WriteMemory(reinterpret_cast<uintptr_t>(match.get<uint8_t>(5)), p, sizeof(p));
        });
    }
}
