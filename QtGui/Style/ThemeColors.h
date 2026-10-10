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
// File: ThemeColors.h
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Applies a plugin style's named colors onto a base style's theme
#pragma once
#include <NoobWarrior/PluginStyle.h>

#include <QColor>

#include <algorithm>
#include <format>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace NoobWarrior {
template <class Theme>
struct ThemeColorName {
    std::string_view Name;
    QColor Theme::*Field;
};

using AssignedThemeColors = std::set<std::string, std::less<>>;

inline StyleColorMap MergeStyleColorsForScheme(const DeclaredStyle &style, Qt::ColorScheme scheme) {
    StyleColorMap colors = style.SharedColors;
    const std::optional<StyleColorMap> &variant = scheme == Qt::ColorScheme::Light ? style.LightColors : style.DarkColors;
    if (variant) {
        for (const auto &[name, color] : *variant)
            colors[name] = color;
    }
    return colors;
}

template <class Theme, std::size_t N>
QColor Theme::*FindThemeColor(const ThemeColorName<Theme> (&names)[N], std::string_view name) {
    const auto *found = std::ranges::find(names, name, &ThemeColorName<Theme>::Name);
    return found != std::end(names) ? found->Field : nullptr;
}

template <class Theme, std::size_t N>
AssignedThemeColors ApplyStyleColors(Theme &theme, const StyleColorMap &colors, const ThemeColorName<Theme> (&names)[N],
                                     std::string_view base, std::vector<std::string> &warnings) {
    AssignedThemeColors assigned;
    for (const auto &[name, color] : colors) {
        QColor Theme::*field = FindThemeColor(names, name);
        if (field == nullptr) {
            warnings.push_back(std::format("\"{}\" is not a color of base \"{}\"", name, base));
            continue;
        }
        theme.*field = QColor(color.R, color.G, color.B, color.A);
        assigned.insert(name);
    }
    return assigned;
}
}
