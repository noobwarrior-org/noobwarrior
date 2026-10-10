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
// File: DarculaTheme.cpp
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Builds a DarculaTheme from a style a plugin declares
#include "DarculaTheme.h"
#include "ThemeColors.h"

using namespace NoobWarrior;

namespace {
constexpr ThemeColorName<DarculaTheme> kColorNames[] = {
    { "window", &DarculaTheme::Window },
    { "base", &DarculaTheme::Base },
    { "alternateBase", &DarculaTheme::AlternateBase },
    { "button", &DarculaTheme::Button },
    { "link", &DarculaTheme::Link },
    { "highlight", &DarculaTheme::Highlight },
    { "toolTipBase", &DarculaTheme::ToolTipBase },
    { "brightText", &DarculaTheme::BrightText },
    { "light", &DarculaTheme::Light },
    { "midlight", &DarculaTheme::Midlight },
    { "dark", &DarculaTheme::Dark },
    { "mid", &DarculaTheme::Mid },
    { "shadow", &DarculaTheme::Shadow },
    { "text", &DarculaTheme::Text },
    { "placeholderText", &DarculaTheme::PlaceholderText },
    { "tabPane", &DarculaTheme::TabPane },
    { "listBase", &DarculaTheme::ListBase },
};
}

std::optional<DarculaTheme> DarculaTheme::FromPluginStyle(const DeclaredStyle &style, Qt::ColorScheme scheme,
                                                          std::vector<std::string> &warnings) {
    if (style.Base != kBaseName)
        return std::nullopt;

    DarculaTheme theme = ForScheme(scheme);
    ApplyStyleColors(theme, MergeStyleColorsForScheme(style, scheme), kColorNames, kBaseName, warnings);
    return theme;
}
