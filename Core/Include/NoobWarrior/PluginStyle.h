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
// File: PluginStyle.h
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Desktop app color styles that plugins declare in their manifest
#pragma once
#include <sol/sol.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
constexpr int kPluginStyleFormatVersion = 1;
constexpr const char *kDefaultPluginStyleBase = "fluent";

struct StyleColor {
    uint8_t R { 0 };
    uint8_t G { 0 };
    uint8_t B { 0 };
    uint8_t A { 255 };
    bool operator==(const StyleColor &) const = default;
};

using StyleColorMap = std::map<std::string, StyleColor>;

struct DeclaredStyle {
    std::string Id;
    std::string Title;
    std::string OwnerIdentifier;
    std::string Base { kDefaultPluginStyleBase };
    StyleColorMap SharedColors;
    std::optional<StyleColorMap> DarkColors;
    std::optional<StyleColorMap> LightColors;
    std::map<std::string, std::string> WebStylesheets;

    std::string GetQualifiedId() const;
    bool SupportsDark() const;
    bool SupportsLight() const;
};

std::optional<StyleColor> ParseStyleColor(const sol::object &value);
StyleColorMap ParseStyleColors(const sol::object &value, const std::string &context, std::vector<std::string> &errors);
bool ParseStyleTable(const sol::object &value, DeclaredStyle &style, std::vector<std::string> &errors);
}
