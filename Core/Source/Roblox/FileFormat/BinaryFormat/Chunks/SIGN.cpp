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
// File: SIGN.cpp
// Started by: Hattozo
// Started on: 8/18/2025
// Description: Signature chunk.

#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/Chunks/SIGN.h>

using namespace NoobWarrior::Roblox;

bool SIGN::Load(BinaryRobloxFileReader &reader) {
    const int32_t count = reader.ReadInt32();
    if (reader.Failed() || count < 0)
        return false;

    Signatures.clear();
    Signatures.reserve(static_cast<size_t>(count));
    for (int32_t index = 0; index < count; ++index) {
        RbxSignature signature;
        signature.SignatureType = static_cast<RbxSignatureType>(reader.ReadInt32());
        signature.PublicKeyId = reader.ReadInt64();
        signature.Value = reader.ReadRawString();
        if (reader.Failed())
            return false;
        Signatures.push_back(std::move(signature));
    }
    return true;
}

void SIGN::Save(BinaryRobloxFileWriter &writer) const {
    writer.WriteInt32(static_cast<int32_t>(Signatures.size()));
    for (const RbxSignature &signature : Signatures) {
        writer.WriteInt32(static_cast<int32_t>(signature.SignatureType));
        writer.WriteInt64(signature.PublicKeyId);
        writer.WriteRawString(signature.Value);
    }
}
