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
// File: Attributes.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/Attributes.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Interfaces/IAttributeToken.h>

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace NoobWarrior::Roblox {
/**
 * @brief One entry of an Instance's AttributesSerialize blob: a type tag plus its decoded value.
 *
 * Reference Tree/Attributes.cs:49-222.
 *
 * Beyond the reference this also keeps the exact bytes the entry occupied in the blob it was
 * loaded from. Re-encoding is lossy in ways that matter to a place file: a CFrame whose matrix a
 * writer spelled out in full even though it is axis-aligned comes back packed into an orient id,
 * and a bool byte of anything other than 0 or 1 comes back as 1. Replaying the original bytes for
 * an entry nobody touched makes a load/save round trip byte-identical rather than merely
 * equivalent, which is what the old engines reading these files need.
 */
class RbxAttribute {
public:
    AttributeType DataType {AttributeType::None};

    /// The decoded value, typed per the token that read it. Mirrors RbxAttribute.Value (object).
    std::any Value;

    /// The bytes this entry occupied, type tag included, or empty for a value never read from one.
    std::vector<unsigned char> Raw;

    /// Set once the value diverges from Raw, which is what forces a re-encode on save.
    bool Dirty {false};

    RbxAttribute() = default;

    /// The value if it is of the requested type, else nullptr -- Attributes.cs:130-137's "is T".
    template <typename T>
    const AttributeValueType<T> *Get() const {
        return std::any_cast<AttributeValueType<T>>(&Value);
    }

    /**
     * @brief Replaces the value, taking the data type from T.
     *
     * Attributes.cs:212-221. Returns false for a type attributes cannot carry; a hard compile
     * error would be the more usual C++ answer, but the reference reports it at runtime and
     * SetAttribute's caller has no other way to ask.
     */
    template <typename T>
    bool Set(T &&value) {
        using Stored = AttributeValueType<T>;
        if constexpr (!SupportedAttributeType<Stored>) {
            return false;
        } else {
            DataType = AttributeTokenOf<Stored>::Token::kAttributeType;
            Value = Stored(std::forward<T>(value));
            Raw.clear();
            Dirty = true;
            return true;
        }
    }

    /**
     * @brief Decodes the type tag and payload at the reader's position.
     *
     * Attributes.cs:170-182. Returns false either because the reader ran dry -- test Failed() to
     * tell which -- or because the tag is one of the types the reference leaves commented out.
     * Nothing is consumed past the point of failure, so a caller can rewind and keep the bytes.
     */
    bool Read(AttributeReader &reader);

    /**
     * @brief Encodes the type tag and payload.
     *
     * Attributes.cs:189-198. Returns false when the stored value does not match DataType, in
     * which case nothing at all is written -- a half-written entry would desynchronise every
     * entry after it.
     */
    bool Write(AttributeWriter &writer) const;

private:
    template <typename Token>
        requires IAttributeToken<Token>
    bool ReadWith(AttributeReader &reader) {
        typename Token::ValueType value = Token::Read(reader);
        if (reader.Failed())
            return false;
        Value = std::move(value);
        return true;
    }

    template <typename Token>
        requires IAttributeToken<Token>
    bool WriteWith(AttributeWriter &writer) const {
        const auto *value = std::any_cast<typename Token::ValueType>(&Value);
        if (value == nullptr)
            return false;
        Token::Write(writer, *value);
        return true;
    }
};

inline bool RbxAttribute::Read(AttributeReader &reader) {
    const uint8_t tag = reader.ReadByte();
    if (reader.Failed())
        return false;

    bool ok = false;
    switch (static_cast<AttributeType>(tag)) {
    case AttributeType::String:         ok = ReadWith<StringAttributeToken>(reader); break;
    case AttributeType::Bool:           ok = ReadWith<BoolAttributeToken>(reader); break;
    case AttributeType::Int:            ok = ReadWith<IntAttributeToken>(reader); break;
    case AttributeType::Float:          ok = ReadWith<FloatAttributeToken>(reader); break;
    case AttributeType::Double:         ok = ReadWith<DoubleAttributeToken>(reader); break;
    case AttributeType::UDim:           ok = ReadWith<UDimAttributeToken>(reader); break;
    case AttributeType::UDim2:          ok = ReadWith<UDim2AttributeToken>(reader); break;
    case AttributeType::BrickColor:     ok = ReadWith<BrickColorAttributeToken>(reader); break;
    case AttributeType::Color3:         ok = ReadWith<Color3AttributeToken>(reader); break;
    case AttributeType::Vector2:        ok = ReadWith<Vector2AttributeToken>(reader); break;
    case AttributeType::Vector3:        ok = ReadWith<Vector3AttributeToken>(reader); break;
    case AttributeType::CFrame:         ok = ReadWith<CFrameAttributeToken>(reader); break;
    case AttributeType::Enum:           ok = ReadWith<EnumAttributeToken>(reader); break;
    case AttributeType::NumberSequence: ok = ReadWith<NumberSequenceAttributeToken>(reader); break;
    case AttributeType::ColorSequence:  ok = ReadWith<ColorSequenceAttributeToken>(reader); break;
    case AttributeType::NumberRange:    ok = ReadWith<NumberRangeAttributeToken>(reader); break;
    case AttributeType::Rect:           ok = ReadWith<RectAttributeToken>(reader); break;
    case AttributeType::FontFace:       ok = ReadWith<FontFaceAttributeToken>(reader); break;
    default:
        // An Array, a Dictionary, a Ray -- a type Roblox writes and this port cannot size, so
        // there is no way to find where the next entry starts. Reported, never guessed at.
        return false;
    }

    if (!ok)
        return false;

    DataType = static_cast<AttributeType>(tag);
    Dirty = false;
    return true;
}

inline bool RbxAttribute::Write(AttributeWriter &writer) const {
    if (!Dirty && !Raw.empty()) {
        writer.WriteRaw(Raw);
        return true;
    }

    // Encoded aside first so a value that turns out not to match its tag leaves the output alone.
    std::string payload;
    AttributeWriter into(payload);

    bool ok = false;
    switch (DataType) {
    case AttributeType::String:         ok = WriteWith<StringAttributeToken>(into); break;
    case AttributeType::Bool:           ok = WriteWith<BoolAttributeToken>(into); break;
    case AttributeType::Int:            ok = WriteWith<IntAttributeToken>(into); break;
    case AttributeType::Float:          ok = WriteWith<FloatAttributeToken>(into); break;
    case AttributeType::Double:         ok = WriteWith<DoubleAttributeToken>(into); break;
    case AttributeType::UDim:           ok = WriteWith<UDimAttributeToken>(into); break;
    case AttributeType::UDim2:          ok = WriteWith<UDim2AttributeToken>(into); break;
    case AttributeType::BrickColor:     ok = WriteWith<BrickColorAttributeToken>(into); break;
    case AttributeType::Color3:         ok = WriteWith<Color3AttributeToken>(into); break;
    case AttributeType::Vector2:        ok = WriteWith<Vector2AttributeToken>(into); break;
    case AttributeType::Vector3:        ok = WriteWith<Vector3AttributeToken>(into); break;
    case AttributeType::CFrame:         ok = WriteWith<CFrameAttributeToken>(into); break;
    case AttributeType::Enum:           ok = WriteWith<EnumAttributeToken>(into); break;
    case AttributeType::NumberSequence: ok = WriteWith<NumberSequenceAttributeToken>(into); break;
    case AttributeType::ColorSequence:  ok = WriteWith<ColorSequenceAttributeToken>(into); break;
    case AttributeType::NumberRange:    ok = WriteWith<NumberRangeAttributeToken>(into); break;
    case AttributeType::Rect:           ok = WriteWith<RectAttributeToken>(into); break;
    case AttributeType::FontFace:       ok = WriteWith<FontFaceAttributeToken>(into); break;
    default:
        return false;
    }

    if (!ok)
        return false;

    writer.WriteByte(static_cast<uint8_t>(DataType));
    writer.WriteRaw(reinterpret_cast<const unsigned char *>(payload.data()), payload.size());
    return true;
}

/**
 * @brief The attribute set of one Instance, and the codec for the blob it is stored in.
 *
 * Reference Tree/Attributes.cs:224-270, where this is a SortedDictionary. Entries are held in a
 * vector in the order the blob listed them instead: re-emitting a foreign blob in a different
 * order would not be the byte-identical round trip an old engine's place file needs, and a new
 * key is inserted at its sorted position so a blob that arrived sorted -- which is how Roblox
 * writes them -- stays sorted.
 */
class RbxAttributes {
public:
    /// Instance.cs:154 refuses a longer key; so does the engine.
    static constexpr size_t kMaxKeyLength = 100;

    enum class Response {
        /// Every entry decoded.
        Ok,
        /// No blob, or one too short to even hold the entry count (Attributes.cs:230-232).
        Empty,
        /// Stopped at a type tag this port cannot size. The rest of the blob is kept verbatim.
        Unsupported,
        /// A length ran past the end of the blob. Nothing is kept; the blob must not be rewritten.
        Malformed,
    };

    struct Entry {
        std::string Key;
        RbxAttribute Attribute;
    };

    /// Attributes.cs:226-246.
    Response Load(std::string_view blob);

    /// Attributes.cs:248-269. Empty when there is nothing to write.
    void Save(std::string &out) const;

    std::string Save() const {
        std::string out;
        Save(out);
        return out;
    }

    void Clear() {
        mEntries.clear();
        mTail.clear();
        mUndecodedCount = 0;
    }

    const RbxAttribute *Find(std::string_view key) const {
        for (const Entry &entry : mEntries)
            if (entry.Key == key)
                return &entry.Attribute;
        return nullptr;
    }

    RbxAttribute *Find(std::string_view key) {
        for (Entry &entry : mEntries)
            if (entry.Key == key)
                return &entry.Attribute;
        return nullptr;
    }

    /// Attributes.cs:224 (the indexer) plus the type and key checks of Instance.cs:152-171.
    template <typename T>
    bool Set(std::string key, T &&value) {
        if (key.size() > kMaxKeyLength)
            return false;

        RbxAttribute attribute;
        if (!attribute.Set(std::forward<T>(value)))
            return false;

        if (RbxAttribute *existing = Find(key)) {
            *existing = std::move(attribute);
            return true;
        }

        const auto at = std::find_if(mEntries.begin(), mEntries.end(),
                                     [&key](const Entry &entry) { return entry.Key > key; });
        mEntries.insert(at, Entry {std::move(key), std::move(attribute)});
        return true;
    }

    bool Remove(std::string_view key) {
        const auto at = std::find_if(mEntries.begin(), mEntries.end(),
                                     [key](const Entry &entry) { return entry.Key == key; });
        if (at == mEntries.end())
            return false;
        mEntries.erase(at);
        return true;
    }

    const std::vector<Entry> &GetEntries() const { return mEntries; }

    /// Decoded entries plus the ones held verbatim, which is what Save writes as the count.
    size_t Count() const { return mEntries.size() + mUndecodedCount; }

    /// True when the blob carried entries of a type this port cannot decode.
    bool HasUndecoded() const { return mUndecodedCount != 0; }

private:
    /**
     * @brief Keeps everything from @p from onwards as opaque bytes and reports why decoding
     * stopped.
     *
     * Attribute entries are only self-delimiting to a decoder that knows every type, so an entry
     * this port cannot read makes every entry after it unreachable too. Dropping them would throw
     * away attributes a user set in Studio, so they are carried through untouched and Save writes
     * them back after the decoded ones.
     */
    Response Stop(std::string_view blob, size_t from, int32_t remainingEntries, Response why) {
        if (why == Response::Malformed) {
            Clear();
            return why;
        }
        mTail.assign(blob.begin() + static_cast<std::ptrdiff_t>(from), blob.end());
        mUndecodedCount = static_cast<size_t>(remainingEntries);
        return why;
    }

    std::vector<Entry> mEntries;
    std::vector<unsigned char> mTail;
    size_t mUndecodedCount {0};
};

inline RbxAttributes::Response RbxAttributes::Load(std::string_view blob) {
    Clear();

    if (blob.size() < 4)
        return Response::Empty;

    AttributeReader reader(blob);
    const int32_t declared = reader.ReadInt();
    if (declared < 0) {
        Clear();
        return Response::Malformed;
    }

    for (int32_t index = 0; index < declared; ++index) {
        const size_t entryStart = reader.Position();

        std::string key = reader.ReadString();
        if (reader.Failed())
            return Stop(blob, entryStart, declared - index, Response::Malformed);

        const size_t valueStart = reader.Position();
        RbxAttribute attribute;
        if (!attribute.Read(reader)) {
            const Response why = reader.Failed() ? Response::Malformed : Response::Unsupported;
            return Stop(blob, entryStart, declared - index, why);
        }

        attribute.Raw.assign(blob.begin() + static_cast<std::ptrdiff_t>(valueStart),
                             blob.begin() + static_cast<std::ptrdiff_t>(reader.Position()));
        mEntries.push_back(Entry {std::move(key), std::move(attribute)});
    }

    // Roblox writes nothing past the last entry, but a blob that has trailing bytes is still a
    // blob a user's place carries; keeping them is what makes Save byte-identical for it too.
    if (reader.Position() < blob.size())
        mTail.assign(blob.begin() + static_cast<std::ptrdiff_t>(reader.Position()), blob.end());

    return mEntries.empty() && mTail.empty() ? Response::Empty : Response::Ok;
}

inline void RbxAttributes::Save(std::string &out) const {
    out.clear();

    if (mEntries.empty() && mUndecodedCount == 0 && mTail.empty())
        return;

    // The entries are encoded before the count is known: an entry whose value no longer matches
    // its type tag writes nothing, and a count that promised it would leave every entry after it
    // being read as part of this one.
    std::string body;
    AttributeWriter into(body);
    size_t written = 0;

    for (const Entry &entry : mEntries) {
        std::string encoded;
        AttributeWriter one(encoded);
        if (!entry.Attribute.Write(one))
            continue;
        into.WriteString(entry.Key);
        into.WriteRaw(reinterpret_cast<const unsigned char *>(encoded.data()), encoded.size());
        ++written;
    }

    AttributeWriter writer(out);
    writer.WriteInt(static_cast<int32_t>(written + mUndecodedCount));
    writer.WriteRaw(reinterpret_cast<const unsigned char *>(body.data()), body.size());
    writer.WriteRaw(mTail);
}
}
