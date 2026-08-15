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
// File: Patches.h
// Started by: Hattozo
// Started on: 3/22/2026
// Description:
#pragma once
#include "../Hook.h"
#include <Hooking.Patterns.h>

namespace NoobHook::Patches {
void RemoveTrustCheck();
void RemoveSignatureCheck();
void RemoveTLSVerification();
void BypassPlaceIdVerification();
void FixSettingsKeyMustBeDefined();
void FixInsertObjects();
// First-chance VEH (x86 only) that lets the 0.573 player survive the 2026-format
// corrupted unions: redirects DeserializedClusterItem::process null-page reads to a
// zero buffer and aborts the corrupt-union CSG mesh builder via its own epilogue.
void InstallClusterNullGuard();
void InstallCrashDiagnostics();
void InstallCrashDiagnostics_LogBacktrace();
// Stopgap (b2) for the 0.574 CSG load-phase heap overflow: hooks the temp-buffer free wrapper
// (RVA 0x22c74d2) and swallows the corrupt free so loading survives. Call after MH_Initialize and
// before MH_EnableHook. x86-only; signature-gated to the 0.574 build.
void InstallCsgHeapGuard();
// EXPERIMENTAL (x86, 0.574 only): flips the new-vertex-format FastFlag (RVA 0x390bb20) so 2026 unions
// take the new render branch (geom+0x1c) instead of the legacy null (geom+0x10) and actually display.
void InstallUnionRenderUnlock();
void InstallTrampolineIntegrityBypass();
}
