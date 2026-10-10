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
// File: PluginStyle.cpp
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Desktop app color styles that plugins declare in their manifest
#include <NoobWarrior/PluginStyle.h>

#include <cmath>
#include <format>
#include <set>

using namespace NoobWarrior;

namespace {
const std::set<std::string> kStyleTableKeys { "version", "base", "colors", "dark", "light", "web" };
const std::set<std::string> kWebStyleTargets { "emu", "master" };
const std::set<std::string> kStyleVariantKeys { "colors" };

std::optional<int64_t> ParseWholeNumber(const sol::object &value) {
    if (value.get_type() != sol::type::number)
        return std::nullopt;
    const double number = value.as<double>();
    if (!std::isfinite(number) || std::floor(number) != number || std::abs(number) > 9007199254740992.0)
        return std::nullopt;
    return static_cast<int64_t>(number);
}

std::optional<uint8_t> ParseChannel(const sol::object &value) {
    const auto number = ParseWholeNumber(value);
    if (!number || *number < 0 || *number > 255)
        return std::nullopt;
    return static_cast<uint8_t>(*number);
}

std::optional<uint8_t> ParseHexByte(char high, char low) {
    const auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    const int h = digit(high), l = digit(low);
    if (h < 0 || l < 0)
        return std::nullopt;
    return static_cast<uint8_t>(h * 16 + l);
}

std::optional<StyleColor> ParseHexColor(const std::string &text) {
    if ((text.size() != 7 && text.size() != 9) || text[0] != '#')
        return std::nullopt;
    uint8_t channels[4] { 0, 0, 0, 255 };
    for (std::size_t i = 0; i < (text.size() - 1) / 2; i++) {
        const auto byte = ParseHexByte(text[1 + i * 2], text[2 + i * 2]);
        if (!byte)
            return std::nullopt;
        channels[i] = *byte;
    }
    return StyleColor { channels[0], channels[1], channels[2], channels[3] };
}

std::optional<StyleColor> ParseArrayColor(const sol::table &table) {
    std::size_t entries = 0;
    for (const auto &pair : table) {
        (void)pair;
        entries++;
    }
    const std::size_t length = table.size();
    if ((length != 3 && length != 4) || entries != length)
        return std::nullopt;
    uint8_t channels[4] { 0, 0, 0, 255 };
    for (std::size_t i = 0; i < length; i++) {
        const auto channel = ParseChannel(table.get<sol::object>(i + 1));
        if (!channel)
            return std::nullopt;
        channels[i] = *channel;
    }
    return StyleColor { channels[0], channels[1], channels[2], channels[3] };
}

void ReportUnknownKeys(const sol::table &table, const std::set<std::string> &known, const std::string &context,
                       std::vector<std::string> &errors) {
    for (const auto &[key, value] : table) {
        if (key.get_type() != sol::type::string)
            errors.push_back(std::format("{} has a key that is not a name, which is ignored", context));
        else if (!known.contains(key.as<std::string>()))
            errors.push_back(std::format("{} has unknown key \"{}\", which is ignored", context, key.as<std::string>()));
    }
}

bool ParseVariant(const sol::table &table, const char *name, std::optional<StyleColorMap> &colors,
                  std::vector<std::string> &errors) {
    const sol::object variant = table.get<sol::object>(name);
    if (variant.get_type() == sol::type::lua_nil)
        return true;
    if (variant.get_type() != sol::type::table) {
        errors.push_back(std::format("\"{}\" must be a table", name));
        return false;
    }
    const sol::table variantTable = variant.as<sol::table>();
    ReportUnknownKeys(variantTable, kStyleVariantKeys, std::format("\"{}\"", name), errors);
    colors = ParseStyleColors(variantTable.get<sol::object>("colors"), std::format("{}.colors", name), errors);
    return true;
}

bool ParseWebStylesheets(const sol::table &table, std::map<std::string, std::string> &stylesheets,
                         std::vector<std::string> &errors) {
    const sol::object web = table.get<sol::object>("web");
    if (web.get_type() == sol::type::lua_nil)
        return true;
    if (web.get_type() != sol::type::table) {
        errors.push_back("\"web\" must be a table of stylesheet paths");
        return false;
    }
    const sol::table webTable = web.as<sol::table>();
    ReportUnknownKeys(webTable, kWebStyleTargets, "\"web\"", errors);
    for (const std::string &target : kWebStyleTargets) {
        const sol::object path = webTable.get<sol::object>(target);
        if (path.get_type() == sol::type::lua_nil)
            continue;
        if (path.get_type() != sol::type::string || path.as<std::string>().empty()) {
            errors.push_back(std::format("\"web.{}\" must be the path of a stylesheet", target));
            continue;
        }
        stylesheets[target] = path.as<std::string>();
    }
    return true;
}
}

std::string DeclaredStyle::GetQualifiedId() const {
    return OwnerIdentifier + "/" + Id;
}

bool DeclaredStyle::SupportsDark() const {
    return DarkColors.has_value() || !LightColors.has_value();
}

bool DeclaredStyle::SupportsLight() const {
    return LightColors.has_value() || !DarkColors.has_value();
}

std::optional<StyleColor> NoobWarrior::ParseStyleColor(const sol::object &value) {
    if (value.get_type() == sol::type::string)
        return ParseHexColor(value.as<std::string>());
    if (value.get_type() == sol::type::table)
        return ParseArrayColor(value.as<sol::table>());
    return std::nullopt;
}

StyleColorMap NoobWarrior::ParseStyleColors(const sol::object &value, const std::string &context,
                                            std::vector<std::string> &errors) {
    StyleColorMap colors;
    if (value.get_type() == sol::type::lua_nil)
        return colors;
    if (value.get_type() != sol::type::table) {
        errors.push_back(std::format("\"{}\" must be a table of colors", context));
        return colors;
    }
    for (const auto &[key, color] : value.as<sol::table>()) {
        if (key.get_type() != sol::type::string) {
            errors.push_back(std::format("\"{}\" has a color without a name, which is ignored", context));
            continue;
        }
        const std::string name = key.as<std::string>();
        const auto parsed = ParseStyleColor(color);
        if (!parsed) {
            errors.push_back(std::format("\"{}.{}\" is not a color; expected {{ r, g, b }}, {{ r, g, b, a }} with "
                                         "whole numbers from 0 to 255, or \"#rrggbb\" / \"#rrggbbaa\"", context, name));
            continue;
        }
        colors[name] = *parsed;
    }
    return colors;
}

bool NoobWarrior::ParseStyleTable(const sol::object &value, DeclaredStyle &style, std::vector<std::string> &errors) {
    if (value.get_type() != sol::type::table) {
        errors.push_back("the style file must return a table");
        return false;
    }
    const sol::table table = value.as<sol::table>();

    const sol::object version = table.get<sol::object>("version");
    if (version.get_type() != sol::type::lua_nil) {
        const auto number = ParseWholeNumber(version);
        if (!number || *number < 1) {
            errors.push_back("\"version\" must be a whole number");
            return false;
        }
        if (*number > kPluginStyleFormatVersion) {
            errors.push_back(std::format("it needs style format version {}, but this noobWarrior only reads up to {}",
                                         *number, kPluginStyleFormatVersion));
            return false;
        }
    }

    const sol::object base = table.get<sol::object>("base");
    if (base.get_type() != sol::type::lua_nil) {
        if (base.get_type() != sol::type::string || base.as<std::string>().empty()) {
            errors.push_back("\"base\" must be the name of a style, such as \"fluent\"");
            return false;
        }
        style.Base = base.as<std::string>();
    }

    ReportUnknownKeys(table, kStyleTableKeys, "the style table", errors);
    style.SharedColors = ParseStyleColors(table.get<sol::object>("colors"), "colors", errors);
    return ParseVariant(table, "dark", style.DarkColors, errors)
        && ParseVariant(table, "light", style.LightColors, errors)
        && ParseWebStylesheets(table, style.WebStylesheets, errors);
}
