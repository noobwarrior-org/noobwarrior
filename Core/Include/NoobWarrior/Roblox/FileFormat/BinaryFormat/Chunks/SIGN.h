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
// File: SIGN.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Signature chunk.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileReader.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryFileWriter.h>

#include <cstdint>
#include <vector>

namespace NoobWarrior::Roblox {
enum class RbxSignatureType : int32_t {
    Ed25519 = 0,
};

struct RbxSignature {
    RbxSignatureType SignatureType {RbxSignatureType::Ed25519};
    int64_t PublicKeyId {};
    std::vector<unsigned char> Value;

    friend bool operator==(const RbxSignature &, const RbxSignature &) = default;
};

// Any edit to the tree invalidates these, so a mutating writer drops the chunk rather than
// emitting a signature that no longer matches.
class SIGN {
public:
    std::vector<RbxSignature> Signatures;

    bool Load(BinaryRobloxFileReader &reader);
    void Save(BinaryRobloxFileWriter &writer) const;
};
}
