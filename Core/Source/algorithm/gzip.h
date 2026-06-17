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
// File: gzip.h
// Started by: Hattozo
// Started on: 6/17/2026
// Description:
#pragma once
#include <vector>
#include <cstddef>
#include <algorithm>

#include <zlib.h>

namespace NoobWarrior {
inline bool IsGzip(const unsigned char* data, size_t size) {
    return size >= 2 && data[0] == 0x1f && data[1] == 0x8b;
}

inline std::vector<unsigned char> GzipInflate(const unsigned char* data, size_t size) {
    std::vector<unsigned char> out;
    if (size == 0)
        return out;

    z_stream strm {};
    if (inflateInit2(&strm, 15 + 32) != Z_OK)
        return out;

    out.resize(std::max<size_t>(size * 4, 65536));
    strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data));
    strm.avail_in = static_cast<uInt>(size);

    int ret;
    do {
        if (strm.total_out >= out.size())
            out.resize(out.size() * 2);
        strm.next_out = reinterpret_cast<Bytef*>(out.data()) + strm.total_out;
        strm.avail_out = static_cast<uInt>(out.size() - strm.total_out);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&strm);
            return {};
        }
    } while (ret != Z_STREAM_END);

    out.resize(strm.total_out);
    inflateEnd(&strm);
    return out;
}

inline void GunzipIfNeeded(std::vector<unsigned char>& data) {
    if (!IsGzip(data.data(), data.size()))
        return;
    std::vector<unsigned char> inflated = GzipInflate(data.data(), data.size());
    if (!inflated.empty())
        data = std::move(inflated);
}
}
