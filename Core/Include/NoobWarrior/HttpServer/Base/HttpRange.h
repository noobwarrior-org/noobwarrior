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
// File: HttpRange.h
// Started by: Hattozo
// Started on: 8/26/2026
// Description:
#pragma once
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>

namespace NoobWarrior {
inline bool ParseHttpRange(const char *header, uint64_t size, uint64_t *outStart, uint64_t *outEnd) {
    if (header == nullptr || size == 0)
        return false;

    std::string value(header);
    constexpr std::string_view kPrefix = "bytes=";
    if (!value.starts_with(kPrefix))
        return false;
    value.erase(0, kPrefix.size());
    if (value.find(',') != std::string::npos)
        return false;

    const size_t dash = value.find('-');
    if (dash == std::string::npos)
        return false;

    const std::string firstText = value.substr(0, dash);
    const std::string lastText  = value.substr(dash + 1);

    auto toNumber = [](const std::string &text, uint64_t *out) {
        if (text.empty())
            return false;
        const char *begin = text.data();
        const char *end   = text.data() + text.size();
        return std::from_chars(begin, end, *out).ec == std::errc();
    };

    uint64_t first = 0, last = 0;
    if (firstText.empty()) {
        // "bytes=-N": the final N bytes.
        if (!toNumber(lastText, &last) || last == 0)
            return false;
        first = last >= size ? 0 : size - last;
        last  = size - 1;
    } else {
        if (!toNumber(firstText, &first))
            return false;
        if (lastText.empty())
            last = size - 1;
        else if (!toNumber(lastText, &last))
            return false;
    }

    if (first >= size)
        return false;
    if (last >= size)
        last = size - 1;
    if (first > last)
        return false;

    *outStart = first;
    *outEnd   = last;
    return true;
}

inline bool IsUnsatisfiableSingleRange(const char *header) {
    if (header == nullptr)
        return false;
    const std::string_view value(header);
    return value.starts_with("bytes=") && value.find(',') == std::string_view::npos;
}
}
