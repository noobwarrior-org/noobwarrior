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
// File: AssetDataFileType.h
// Started by: Hattozo
// Started on: 6/17/2026
// Description:
#pragma once
#include <QString>

#include <vector>
#include <cstring>

namespace NoobWarrior {
inline QString DetectAssetExtension(const std::vector<unsigned char> &data) {
    auto starts = [&](const char *sig, size_t len) {
        if (data.size() < len)
            return false;
        return std::memcmp(data.data(), sig, len) == 0;
    };

    if (starts("<roblox!", 8))
        return "rbxm";
    if (starts("<roblox", 7))
        return "rbxmx";
    if (starts("version ", 8))
        return "mesh";

    if (starts("\x89PNG\r\n\x1a\n", 8))
        return "png";
    if (starts("\xff\xd8\xff", 3))
        return "jpg";
    if (starts("GIF87a", 6) || starts("GIF89a", 6))
        return "gif";
    if (starts("BM", 2))
        return "bmp";
    if (data.size() >= 12 && starts("RIFF", 4) && std::memcmp(data.data() + 8, "WEBP", 4) == 0)
        return "webp";
    if (data.size() >= 12 && starts("RIFF", 4) && std::memcmp(data.data() + 8, "WAVE", 4) == 0)
        return "wav";
    if (starts("OggS", 4))
        return "ogg";
    if (starts("ID3", 3) || starts("\xff\xfb", 2) || starts("\xff\xf3", 2) || starts("\xff\xf2", 2))
        return "mp3";
    if (starts("fLaC", 4))
        return "flac";
    if (starts("\xabKTX 11\xbb\r\n\x1a\n", 12))
        return "ktx";
    if (starts("DDS ", 4))
        return "dds";
    if (starts("Kaydara FBX Binary", 18))
        return "fbx";
    
    if (data.size() >= 12 && std::memcmp(data.data() + 4, "ftyp", 4) == 0)
        return "mp4";
    if (starts("\x1a\x45\xdf\xa3", 4)) // EBML header: WebM / Matroska
        return "webm";

    if (starts("%PDF", 4))
        return "pdf";
    if (starts("PK\x03\x04", 4))
        return "zip";

    return "bin";
}
}
