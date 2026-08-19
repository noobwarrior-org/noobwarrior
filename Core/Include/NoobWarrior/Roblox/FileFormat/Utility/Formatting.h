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
// File: Formatting.h
// Description: Ported from RobloxFiles.Formatting: invariant number parsing and
//              formatting, plus the few pugixml node helpers the tokens share.
#pragma once

#include <pugixml.hpp>

#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox::Tokens {
// Roblox writes INF/NAN in a few numeric fields, which from_chars rejects, so those are handled
// explicitly before falling back to the locale-independent parser.
inline bool ParseFloat(std::string_view text, float &value) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
        text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
        text.remove_suffix(1);
    if (text == "INF" || text == "inf") { value = HUGE_VALF; return true; }
    if (text == "-INF" || text == "-inf") { value = -HUGE_VALF; return true; }
    if (text == "NAN" || text == "nan" || text == "-NAN" || text == "-nan") {
        value = NAN;
        return true;
    }
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc {} && parsed.ptr == text.data() + text.size();
}

inline bool ParseDouble(std::string_view text, double &value) {
    float single = 0;
    if (text == "INF" || text == "-INF" || text == "NAN") {
        ParseFloat(text, single);
        value = single;
        return true;
    }
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc {};
}

template<typename T>
bool ParseInteger(std::string_view text, T &value, int base = 10) {
    while (!text.empty() && text.front() == ' ')
        text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\r'))
        text.remove_suffix(1);
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value, base);
    return parsed.ec == std::errc {};
}

// The fallback is a parameter because a missing child does not always mean zero: RobloxFiles
// reads PhysicalProperties' children with a fallback of 1 (Tokens/PhysicalProperties.cs:28) while
// every other caller wants 0.
inline float ChildFloat(const pugi::xml_node &node, const char *name, float fallback = 0.0f) {
    const pugi::xml_node child = node.child(name);
    if (!child)
        return fallback;
    float value = fallback;
    if (!ParseFloat(child.text().as_string(), value))
        return fallback;
    return value;
}

inline std::vector<std::string_view> SplitOnSpaces(std::string_view text) {
    std::vector<std::string_view> parts;
    size_t index = 0;
    while (index < text.size()) {
        while (index < text.size() && (text[index] == ' ' || text[index] == '\n' ||
                                       text[index] == '\r' || text[index] == '\t')) {
            ++index;
        }
        const size_t start = index;
        while (index < text.size() && text[index] != ' ' && text[index] != '\n' &&
               text[index] != '\r' && text[index] != '\t') {
            ++index;
        }
        if (index > start)
            parts.push_back(text.substr(start, index - start));
    }
    return parts;
}

// Roblox emits floats round-trippably; std::to_chars gives the shortest exact representation.
inline std::string FormatFloat(float value) {
    if (std::isinf(value))
        return value > 0 ? "INF" : "-INF";
    if (std::isnan(value))
        return "NAN";
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

// Same treatment for doubles. std::to_string would clamp to six decimals and has no spelling for
// INF/NAN, both of which RobloxFiles emits (Utility/Formatting.cs:26-38).
inline std::string FormatDouble(double value) {
    if (std::isinf(value))
        return value > 0 ? "INF" : "-INF";
    if (std::isnan(value))
        return "NAN";
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

// Utility/Formatting.cs:105-108. Roblox rounds coordinates on its own way in and out of a file, so
// comparisons against a written value have to allow the last few digits to disagree.
inline bool FuzzyEquals(float a, float b, float epsilon = 1e-4f) {
    return std::fabs(a - b) < epsilon;
}

inline void WriteChildFloat(pugi::xml_node node, const char *name, float value) {
    node.append_child(name).text().set(FormatFloat(value).c_str());
}

inline void SetText(pugi::xml_node node, const std::string &text) {
    // A value containing newlines is written as CDATA, matching RobloxFiles.
    if (text.find('\n') != std::string::npos || text.find('\r') != std::string::npos)
        node.append_child(pugi::node_cdata).set_value(text.c_str());
    else
        node.text().set(text.c_str());
}

// A BinaryString's payload is base64 in the XML form (Tokens/BinaryString.cs:12-14) but raw bytes
// everywhere else, so the two representations have to be converted rather than passed through.
// Returns false on anything that is not well-formed base64, which leaves the property untouched.
inline bool Base64Decode(std::string_view text, std::vector<unsigned char> &output) {
    const auto sextet = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    output.clear();
    output.reserve((text.size() / 4) * 3);
    uint32_t accumulator = 0;
    int bits = 0;
    bool padded = false;
    for (const char c : text) {
        // RobloxFiles wraps the payload every 72 characters, so whitespace is expected.
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
            continue;
        if (c == '=') {
            padded = true;
            continue;
        }
        if (padded)
            return false;
        const int value = sextet(c);
        if (value < 0)
            return false;
        accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output.push_back(static_cast<unsigned char>((accumulator >> bits) & 0xffu));
        }
    }
    // Whatever is left over is padding, and padding bits are zero in a well-formed stream.
    return (accumulator & ((1u << bits) - 1u)) == 0;
}

// std::string is the port's carrier for String-typed property values, and it holds arbitrary bytes,
// so it is what the encoder takes.
inline std::string Base64Encode(std::string_view data) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const auto byte = [&data](size_t index) {
        return static_cast<uint32_t>(static_cast<unsigned char>(data[index]));
    };
    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);
    size_t index = 0;
    for (; index + 3 <= data.size(); index += 3) {
        const uint32_t triple = (byte(index) << 16) | (byte(index + 1) << 8) | byte(index + 2);
        encoded.push_back(kAlphabet[(triple >> 18) & 0x3f]);
        encoded.push_back(kAlphabet[(triple >> 12) & 0x3f]);
        encoded.push_back(kAlphabet[(triple >> 6) & 0x3f]);
        encoded.push_back(kAlphabet[triple & 0x3f]);
    }
    if (index + 1 == data.size()) {
        const uint32_t triple = byte(index) << 16;
        encoded.push_back(kAlphabet[(triple >> 18) & 0x3f]);
        encoded.push_back(kAlphabet[(triple >> 12) & 0x3f]);
        encoded.append("==");
    } else if (index + 2 == data.size()) {
        const uint32_t triple = (byte(index) << 16) | (byte(index + 1) << 8);
        encoded.push_back(kAlphabet[(triple >> 18) & 0x3f]);
        encoded.push_back(kAlphabet[(triple >> 12) & 0x3f]);
        encoded.push_back(kAlphabet[(triple >> 6) & 0x3f]);
        encoded.push_back('=');
    }
    return encoded;
}
}
