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
// File: DesktopSettingsFrame.h
// Started by: Hattozo
// Started on: 8/26/2026
// Description:
#pragma once
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Paths.h>

#include <zstd.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace NoobWarrior {
constexpr const char *kDesktopSettingsDictionary =
    "881010b3c0e6682563bcca18fa7e762b3dae2e81e7b5dc72cc01aa2d26aca6b5.dict";

inline std::optional<std::vector<char>> FindDesktopSettingsDictionary(Core *core) {
    if (core == nullptr)
        return std::nullopt;

    std::error_code error;
    const std::filesystem::path enginesDir = core->GetUserDataDir() / NW_PATH_ENGINES;
    if (!std::filesystem::exists(enginesDir, error))
        return std::nullopt;

    for (const auto &entry : std::filesystem::directory_iterator(enginesDir, error)) {
        if (!entry.is_directory(error))
            continue;
        const std::filesystem::path path = entry.path() / "PlatformContent" / "pc" /
            "shared_compression_dictionaries" / kDesktopSettingsDictionary;
        std::ifstream file(path, std::ios::binary);
        if (!file)
            continue;
        return std::vector<char>{std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>()};
    }
    return std::nullopt;
}

inline std::optional<std::string> DecompressWithDictionary(const unsigned char *data, std::size_t size,
                                                           const std::vector<char> &dictionary) {
    const unsigned long long contentSize = ZSTD_getFrameContentSize(data, size);
    if (contentSize == ZSTD_CONTENTSIZE_ERROR || contentSize == ZSTD_CONTENTSIZE_UNKNOWN)
        return std::nullopt;

    std::string plain(static_cast<std::size_t>(contentSize), '\0');
    ZSTD_DCtx *context = ZSTD_createDCtx();
    if (context == nullptr)
        return std::nullopt;
    const std::size_t result = ZSTD_decompress_usingDict(
        context, plain.data(), plain.size(), data, size, dictionary.data(), dictionary.size());
    ZSTD_freeDCtx(context);
    if (ZSTD_isError(result) || result != plain.size())
        return std::nullopt;
    return plain;
}

inline std::optional<std::string> CompressWithDictionary(const std::string &plain,
                                                         const std::vector<char> &dictionary) {
    std::string out(ZSTD_compressBound(plain.size()), '\0');
    ZSTD_CCtx *context = ZSTD_createCCtx();
    if (context == nullptr)
        return std::nullopt;
    const std::size_t written = ZSTD_compress_usingDict(
        context, out.data(), out.size(), plain.data(), plain.size(),
        dictionary.data(), dictionary.size(), ZSTD_defaultCLevel());
    ZSTD_freeCCtx(context);
    if (ZSTD_isError(written))
        return std::nullopt;
    out.resize(written);
    return out;
}
}
