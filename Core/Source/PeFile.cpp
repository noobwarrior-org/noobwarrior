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
// File: PeFile.cpp
// Started by: Hattozo
// Started on: 8/5/2026
// Description: Implementation of the PE metadata reader
//              (Disclaimer: This code was created by Claude Opus 5.)
#include <NoobWarrior/PeFile.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>

using namespace NoobWarrior;

// Everything here parses the file by hand rather than going through GetFileVersionInfo and friends,
// so engines are identified identically on every host OS (on Linux/macOS the Win32 API isn't there
// at all, which used to leave every version empty).
namespace {

// Random-access reader over the executable; only the few hundred bytes we actually need are read,
// engine binaries are far too big to slurp whole.
class PeReader {
public:
    explicit PeReader(const std::filesystem::path &path) : mStream(path, std::ios::binary) {
        if (mStream.is_open()) {
            mStream.seekg(0, std::ios::end);
            mSize = static_cast<uint64_t>(mStream.tellg());
        }
    }

    bool IsOpen() const { return mStream.is_open(); }

    bool ReadAt(uint64_t offset, void *dst, size_t len) {
        if (offset > mSize || mSize - offset < len) return false;
        mStream.clear();
        mStream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        mStream.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(len));
        return mStream.gcount() == static_cast<std::streamsize>(len);
    }

    // PE integers are little-endian regardless of the host, so assemble them byte by byte.
    template <typename T>
    bool ReadInt(uint64_t offset, T &out) {
        uint8_t bytes[sizeof(T)];
        if (!ReadAt(offset, bytes, sizeof(bytes))) return false;
        T value = 0;
        for (size_t i = 0; i < sizeof(T); i++)
            value |= static_cast<T>(bytes[i]) << (8 * i);
        out = value;
        return true;
    }

private:
    std::ifstream mStream;
    uint64_t      mSize {0};
};

struct PeSection {
    uint32_t VirtualAddress;
    uint32_t VirtualSize;
    uint32_t RawAddress;
    uint32_t RawSize;
};

// Section headers, plus the RVA/size of one data directory entry (we only ever want the resources).
struct PeLayout {
    std::vector<PeSection> Sections;
    uint32_t DirectoryRva  {0};
    uint32_t DirectorySize {0};
};

constexpr uint16_t kDosSignature           = 0x5A4D;     // "MZ"
constexpr uint32_t kPeSignature            = 0x00004550; // "PE\0\0"
constexpr uint32_t kResourceDirectoryIndex = 2;
constexpr uint16_t kResourceTypeVersion    = 16; // RT_VERSION

// Checks the two signatures and hands back the offset of the PE header the DOS stub points at.
static bool ReadPeOffset(PeReader &pe, uint32_t &peOffset) {
    uint16_t mz = 0;
    if (!pe.ReadInt(0, mz) || mz != kDosSignature) return false;
    if (!pe.ReadInt(0x3C, peOffset)) return false;

    uint32_t signature = 0;
    return pe.ReadInt(peOffset, signature) && signature == kPeSignature;
}

static bool ReadPeLayout(PeReader &pe, PeLayout &out) {
    uint32_t peOffset = 0;
    if (!ReadPeOffset(pe, peOffset)) return false;

    const uint64_t coff = peOffset + 4;
    uint16_t sectionCount = 0, optionalHeaderSize = 0;
    if (!pe.ReadInt(coff + 2, sectionCount)) return false;
    if (!pe.ReadInt(coff + 16, optionalHeaderSize)) return false;

    const uint64_t optional = coff + 20;
    uint16_t magic = 0;
    if (!pe.ReadInt(optional, magic)) return false;

    // PE32 keeps the data directories at 0x60, PE32+ at 0x70 (its extra 64-bit fields shift them).
    uint64_t directoryCountOffset;
    uint64_t directoriesOffset;
    if (magic == 0x010B) {
        directoryCountOffset = optional + 92;
        directoriesOffset    = optional + 96;
    } else if (magic == 0x020B) {
        directoryCountOffset = optional + 108;
        directoriesOffset    = optional + 112;
    } else {
        return false;
    }

    uint32_t directoryCount = 0;
    if (!pe.ReadInt(directoryCountOffset, directoryCount)) return false;
    if (directoryCount <= kResourceDirectoryIndex) return false;

    if (!pe.ReadInt(directoriesOffset + kResourceDirectoryIndex * 8, out.DirectoryRva)) return false;
    if (!pe.ReadInt(directoriesOffset + kResourceDirectoryIndex * 8 + 4, out.DirectorySize)) return false;
    if (out.DirectoryRva == 0 || out.DirectorySize == 0) return false;

    const uint64_t sectionTable = optional + optionalHeaderSize;
    for (uint16_t i = 0; i < sectionCount; i++) {
        const uint64_t header = sectionTable + i * 40ull;
        PeSection section {};
        if (!pe.ReadInt(header + 8,  section.VirtualSize))    return false;
        if (!pe.ReadInt(header + 12, section.VirtualAddress)) return false;
        if (!pe.ReadInt(header + 16, section.RawSize))        return false;
        if (!pe.ReadInt(header + 20, section.RawAddress))     return false;
        out.Sections.push_back(section);
    }
    return !out.Sections.empty();
}

static std::optional<uint64_t> RvaToFileOffset(const PeLayout &layout, uint32_t rva) {
    for (const auto &section : layout.Sections) {
        const uint32_t span = std::max(section.VirtualSize, section.RawSize);
        if (rva >= section.VirtualAddress && rva < section.VirtualAddress + span) {
            const uint32_t delta = rva - section.VirtualAddress;
            if (delta >= section.RawSize) return std::nullopt; // lives in uninitialised padding
            return static_cast<uint64_t>(section.RawAddress) + delta;
        }
    }
    return std::nullopt;
}

// Walks one level of the resource tree. Pass wantId to select an entry by id, or std::nullopt to
// take the first entry there is (used for the language/name levels, where any match will do).
// Returns the offset of the child node relative to the start of the resource directory, and whether
// that child is another directory or a leaf data entry.
static bool FindResourceEntry(PeReader &pe, uint64_t directoryOffset, std::optional<uint16_t> wantId,
                              uint32_t &childOffset, bool &childIsDirectory) {
    uint16_t namedCount = 0, idCount = 0;
    if (!pe.ReadInt(directoryOffset + 12, namedCount)) return false;
    if (!pe.ReadInt(directoryOffset + 14, idCount)) return false;

    const uint32_t total = static_cast<uint32_t>(namedCount) + idCount;
    for (uint32_t i = 0; i < total; i++) {
        const uint64_t entry = directoryOffset + 16 + i * 8ull;
        uint32_t name = 0, offsetToData = 0;
        if (!pe.ReadInt(entry, name)) return false;
        if (!pe.ReadInt(entry + 4, offsetToData)) return false;

        if (wantId.has_value()) {
            const bool isNamed = (name & 0x80000000u) != 0;
            if (isNamed || static_cast<uint16_t>(name) != *wantId) continue;
        }

        childIsDirectory = (offsetToData & 0x80000000u) != 0;
        childOffset      = offsetToData & 0x7FFFFFFFu;
        return true;
    }
    return false;
}

// Pulls the RT_VERSION resource (the VS_VERSIONINFO blob) out of the file.
static bool ReadVersionResource(PeReader &pe, const PeLayout &layout, std::vector<uint8_t> &blob) {
    auto rootOffset = RvaToFileOffset(layout, layout.DirectoryRva);
    if (!rootOffset) return false;

    uint32_t offset = 0;
    bool isDirectory = false;
    if (!FindResourceEntry(pe, *rootOffset, kResourceTypeVersion, offset, isDirectory) || !isDirectory)
        return false;
    if (!FindResourceEntry(pe, *rootOffset + offset, std::nullopt, offset, isDirectory) || !isDirectory)
        return false;
    if (!FindResourceEntry(pe, *rootOffset + offset, std::nullopt, offset, isDirectory) || isDirectory)
        return false;

    // Leaf: IMAGE_RESOURCE_DATA_ENTRY. Its OffsetToData is an RVA, not a resource-relative offset.
    uint32_t dataRva = 0, dataSize = 0;
    if (!pe.ReadInt(*rootOffset + offset, dataRva)) return false;
    if (!pe.ReadInt(*rootOffset + offset + 4, dataSize)) return false;
    if (dataSize == 0 || dataSize > 1024 * 1024) return false;

    auto dataOffset = RvaToFileOffset(layout, dataRva);
    if (!dataOffset) return false;

    blob.resize(dataSize);
    return pe.ReadAt(*dataOffset, blob.data(), blob.size());
}

// One node of the VS_VERSIONINFO tree: a length-prefixed header, a UTF-16 key, an optional value,
// then child nodes - every part padded up to a 4-byte boundary.
struct VersionNode {
    uint16_t    ValueLength   {0};
    uint16_t    Type          {0}; // 1 = text value, 0 = binary
    std::string Key           {};
    size_t      ValueOffset   {0};
    size_t      ChildrenOffset{0};
    size_t      End           {0};
};

static size_t Align4(size_t value) { return (value + 3) & ~static_cast<size_t>(3); }

static uint16_t ReadU16(const std::vector<uint8_t> &blob, size_t offset) {
    return static_cast<uint16_t>(blob[offset] | (blob[offset + 1] << 8));
}

static bool ReadVersionNode(const std::vector<uint8_t> &blob, size_t offset, VersionNode &out) {
    if (offset + 6 > blob.size()) return false;

    const uint16_t length = ReadU16(blob, offset);
    out.ValueLength = ReadU16(blob, offset + 2);
    out.Type        = ReadU16(blob, offset + 4);
    if (length < 6 || offset + length > blob.size()) return false;
    out.End = offset + length;

    // Keys are always plain ASCII in practice ("ProductVersion", "StringFileInfo", ...), so keep the
    // low byte of each UTF-16 unit and let anything else fail the comparisons it is used for.
    size_t cursor = offset + 6;
    while (cursor + 2 <= out.End) {
        const uint16_t unit = ReadU16(blob, cursor);
        cursor += 2;
        if (unit == 0) break;
        out.Key += (unit < 0x80) ? static_cast<char>(unit) : '?';
    }

    out.ValueOffset = Align4(cursor);
    // A text value counts UTF-16 units, a binary one counts bytes.
    size_t valueBytes = out.Type == 1 ? static_cast<size_t>(out.ValueLength) * 2 : out.ValueLength;
    if (out.ValueOffset + valueBytes > out.End)
        valueBytes = out.ValueOffset < out.End ? out.End - out.ValueOffset : 0;
    out.ChildrenOffset = Align4(out.ValueOffset + valueBytes);
    return true;
}

static std::string ReadNodeTextValue(const std::vector<uint8_t> &blob, const VersionNode &node) {
    std::string value;
    for (size_t i = node.ValueOffset; i + 2 <= node.End; i += 2) {
        const uint16_t unit = ReadU16(blob, i);
        if (unit == 0) break;
        value += (unit < 0x80) ? static_cast<char>(unit) : '?';
    }
    return value;
}

// "0, 463, 0, 417004" -> "0.463.0.417004"
static std::string NormalizeVersion(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (char c : raw) {
        if (c == ' ' || c == '\0') continue;
        out += (c == ',') ? '.' : c;
    }
    return out;
}

// Digs "ProductVersion" out of any of the StringFileInfo translation tables.
static std::string FindProductVersion(const std::vector<uint8_t> &blob) {
    VersionNode root {};
    if (!ReadVersionNode(blob, 0, root) || root.Key != "VS_VERSION_INFO") return "";

    for (size_t i = root.ChildrenOffset; i < root.End; ) {
        VersionNode fileInfo {};
        if (!ReadVersionNode(blob, i, fileInfo) || fileInfo.End <= i) break;
        i = Align4(fileInfo.End);
        if (fileInfo.Key != "StringFileInfo") continue;

        for (size_t j = fileInfo.ChildrenOffset; j < fileInfo.End; ) {
            VersionNode table {}; // one per language/codepage, e.g. "040904b0"
            if (!ReadVersionNode(blob, j, table) || table.End <= j) break;
            j = Align4(table.End);

            for (size_t k = table.ChildrenOffset; k < table.End; ) {
                VersionNode entry {};
                if (!ReadVersionNode(blob, k, entry) || entry.End <= k) break;
                k = Align4(entry.End);
                if (entry.Key == "ProductVersion") {
                    std::string value = NormalizeVersion(ReadNodeTextValue(blob, entry));
                    if (!value.empty()) return value;
                }
            }
        }
    }
    return "";
}

// Last resort: the numeric VS_FIXEDFILEINFO. Each field is 16 bits, so a build number above 65535
// (Roblox uses those) is truncated - which is why the string table is preferred.
static std::string FixedFileInfoVersion(const std::vector<uint8_t> &blob) {
    VersionNode root {};
    if (!ReadVersionNode(blob, 0, root) || root.ValueLength < 52) return "";
    if (root.ValueOffset + 52 > blob.size()) return "";

    auto u32 = [&](size_t offset) {
        return static_cast<uint32_t>(ReadU16(blob, offset)) |
               (static_cast<uint32_t>(ReadU16(blob, offset + 2)) << 16);
    };
    if (u32(root.ValueOffset) != 0xFEEF04BDu) return "";

    const uint32_t ms = u32(root.ValueOffset + 16); // dwProductVersionMS
    const uint32_t ls = u32(root.ValueOffset + 20); // dwProductVersionLS
    return std::to_string(ms >> 16) + "." + std::to_string(ms & 0xFFFF) + "." +
           std::to_string(ls >> 16) + "." + std::to_string(ls & 0xFFFF);
}

} // namespace

namespace NoobWarrior::Pe {

Machine ReadMachine(const std::filesystem::path &path) {
    PeReader pe(path);
    if (!pe.IsOpen()) return Machine::Unknown;

    uint32_t peOffset = 0;
    if (!ReadPeOffset(pe, peOffset)) return Machine::Unknown;

    // First field of the COFF header, which sits right behind the signature.
    uint16_t machine = 0;
    if (!pe.ReadInt(peOffset + 4, machine)) return Machine::Unknown;

    switch (machine) {
    case 0x014C: return Machine::x86;
    case 0x8664: return Machine::x86_64;
    default:     return Machine::Unknown;
    }
}

std::string ReadProductVersion(const std::filesystem::path &path) {
    PeReader pe(path);
    if (!pe.IsOpen()) return "";

    PeLayout layout {};
    if (!ReadPeLayout(pe, layout)) return "";

    std::vector<uint8_t> blob;
    if (!ReadVersionResource(pe, layout, blob)) return "";

    std::string version = FindProductVersion(blob);
    if (version.empty())
        version = FixedFileInfoVersion(blob);
    return version;
}

}
