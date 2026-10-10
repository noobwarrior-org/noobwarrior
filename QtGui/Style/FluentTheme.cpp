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
// File: FluentTheme.cpp
// Started by: Hattozo
// Started on: 10/10/2026
// Description: Builds a FluentTheme from a style a plugin declares
#include "FluentTheme.h"

#include "ThemeColors.h"

#include <algorithm>
#include <string_view>

using namespace NoobWarrior;

namespace {
constexpr ThemeColorName<FluentTheme> kColorNames[] = {
    { "shell", &FluentTheme::Shell },
    { "pane", &FluentTheme::Pane },
    { "alternateBase", &FluentTheme::AlternateBase },
    { "menu", &FluentTheme::Menu },
    { "menuEdge", &FluentTheme::MenuEdge },
    { "menuHighlight", &FluentTheme::MenuHighlight },
    { "menuBarHover", &FluentTheme::MenuBarHover },
    { "control", &FluentTheme::Control },
    { "controlHover", &FluentTheme::ControlHover },
    { "controlPressed", &FluentTheme::ControlPressed },
    { "controlDisabled", &FluentTheme::ControlDisabled },
    { "controlBorder", &FluentTheme::ControlBorder },
    { "controlStroke", &FluentTheme::ControlStroke },
    { "indicatorFill", &FluentTheme::IndicatorFill },
    { "indicatorBorder", &FluentTheme::IndicatorBorder },
    { "border", &FluentTheme::Border },
    { "subtleBorder", &FluentTheme::SubtleBorder },
    { "bevelLight", &FluentTheme::BevelLight },
    { "bevelMidlight", &FluentTheme::BevelMidlight },
    { "bevelDark", &FluentTheme::BevelDark },
    { "rowHover", &FluentTheme::RowHover },
    { "tabHover", &FluentTheme::TabHover },
    { "tabSelected", &FluentTheme::TabSelected },
    { "selection", &FluentTheme::Selection },
    { "accent", &FluentTheme::Accent },
    { "accentHover", &FluentTheme::AccentHover },
    { "accentPressed", &FluentTheme::AccentPressed },
    { "onAccent", &FluentTheme::OnAccent },
    { "textSelection", &FluentTheme::TextSelection },
    { "textOnSelection", &FluentTheme::TextOnSelection },
    { "toolBar", &FluentTheme::ToolBar },
    { "toolBarEdge", &FluentTheme::ToolBarEdge },
    { "toolBarGrip", &FluentTheme::ToolBarGrip },
    { "captionHover", &FluentTheme::CaptionHover },
    { "captionPressed", &FluentTheme::CaptionPressed },
    { "captionGlyphHover", &FluentTheme::CaptionGlyphHover },
    { "closeGlyphHover", &FluentTheme::CloseGlyphHover },
    { "closeHover", &FluentTheme::CloseHover },
    { "closePressed", &FluentTheme::ClosePressed },
    { "text", &FluentTheme::Text },
    { "textSecondary", &FluentTheme::TextSecondary },
    { "textPlaceholder", &FluentTheme::TextPlaceholder },
    { "textDisabled", &FluentTheme::TextDisabled },
    { "scrollThumb", &FluentTheme::ScrollThumb },
    { "scrollThumbHover", &FluentTheme::ScrollThumbHover },
};

struct Derivation {
    std::string_view Target;
    std::string_view Source;
    int DarkShift;
    int LightShift;
};

constexpr Derivation kDerivations[] = {
    { "controlHover", "control", 8, -7 },
    { "controlPressed", "control", -7, -13 },
    { "controlDisabled", "control", -10, -8 },
    { "accentHover", "accent", 14, 14 },
    { "accentPressed", "accent", -22, -22 },
    { "menuHighlight", "menu", 17, -17 },
    { "rowHover", "pane", 7, -16 },
    { "menuBarHover", "shell", 17, -12 },
    { "captionHover", "shell", 17, -9 },
    { "captionPressed", "shell", 13, -18 },
    { "scrollThumbHover", "scrollThumb", 52, -46 },
};

QColor Shift(const QColor &color, int amount) {
    const auto channel = [amount](int value) { return std::clamp(value + amount, 0, 255); };
    return QColor(channel(color.red()), channel(color.green()), channel(color.blue()), color.alpha());
}
}

std::optional<FluentTheme> FluentTheme::FromPluginStyle(const DeclaredStyle &style, Qt::ColorScheme scheme,
                                                        std::vector<std::string> &warnings) {
    if (style.Base != kBaseName)
        return std::nullopt;

    FluentTheme theme = ForScheme(scheme);
    const AssignedThemeColors assigned = ApplyStyleColors(theme, MergeStyleColorsForScheme(style, scheme), kColorNames,
                                                          kBaseName, warnings);
    for (const Derivation &derivation : kDerivations) {
        if (!assigned.contains(derivation.Source) || assigned.contains(derivation.Target))
            continue;
        const int shift = scheme == Qt::ColorScheme::Light ? derivation.LightShift : derivation.DarkShift;
        theme.*FindThemeColor(kColorNames, derivation.Target) = Shift(theme.*FindThemeColor(kColorNames, derivation.Source), shift);
    }

    return theme;
}
