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
// File: FluentTheme.h
// Started by: Hattozo
// Started on: 10/10/2026
// Description: The set of colors FluentStyle paints with
#pragma once
#include <QColor>

#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
struct DeclaredStyle;

struct FluentTheme {
    static constexpr const char *kBaseName = "fluent";

    Qt::ColorScheme Scheme;
    QColor Shell;
    QColor Pane;
    QColor AlternateBase;
    QColor Menu;
    QColor MenuEdge;
    QColor MenuHighlight;
    QColor MenuBarHover;
    QColor Control;
    QColor ControlHover;
    QColor ControlPressed;
    QColor ControlDisabled;
    QColor ControlBorder;
    QColor ControlStroke;
    QColor IndicatorFill;
    QColor IndicatorBorder;
    QColor Border;
    QColor SubtleBorder;
    QColor BevelLight;
    QColor BevelMidlight;
    QColor BevelDark;
    QColor RowHover;
    QColor TabHover;
    QColor TabSelected;
    QColor Selection;
    QColor Accent;
    QColor AccentHover;
    QColor AccentPressed;
    QColor OnAccent;
    QColor TextSelection;
    QColor TextOnSelection;
    QColor ToolBar;
    QColor ToolBarEdge;
    QColor ToolBarGrip;
    QColor CaptionHover;
    QColor CaptionPressed;
    QColor CaptionGlyphHover;
    QColor CloseGlyphHover;
    QColor CloseHover;
    QColor ClosePressed;
    QColor Text;
    QColor TextSecondary;
    QColor TextPlaceholder;
    QColor TextDisabled;
    QColor ScrollThumb;
    QColor ScrollThumbHover;

    static FluentTheme Dark() {
        return {
            .Scheme = Qt::ColorScheme::Dark,
            .Shell = QColor(0x1c, 0x1c, 0x1c),
            .Pane = QColor(0x28, 0x28, 0x28),
            .AlternateBase = QColor(0x2c, 0x2c, 0x2c),
            .Menu = QColor(0x2c, 0x2c, 0x2c),
            .MenuEdge = QColor(0x14, 0x14, 0x14),
            .MenuHighlight = QColor(0x3d, 0x3d, 0x3d),
            .MenuBarHover = QColor(0x2d, 0x2d, 0x2d),
            .Control = QColor(0x35, 0x35, 0x35),
            .ControlHover = QColor(0x3d, 0x3d, 0x3d),
            .ControlPressed = QColor(0x2e, 0x2e, 0x2e),
            .ControlDisabled = QColor(0x2b, 0x2b, 0x2b),
            .ControlBorder = QColor(0x42, 0x42, 0x42),
            .ControlStroke = QColor(0x4a, 0x4a, 0x4a),
            .IndicatorFill = QColor(0x24, 0x24, 0x24),
            .IndicatorBorder = QColor(0x8a, 0x8a, 0x8a),
            .Border = QColor(0x3d, 0x3d, 0x3d),
            .SubtleBorder = QColor(0x33, 0x33, 0x33),
            .BevelLight = QColor(0x40, 0x40, 0x40),
            .BevelMidlight = QColor(0x38, 0x38, 0x38),
            .BevelDark = QColor(0x1a, 0x1a, 0x1a),
            .RowHover = QColor(0x2f, 0x2f, 0x2f),
            .TabHover = QColor(0x33, 0x33, 0x33),
            .TabSelected = QColor(0x2e, 0x2e, 0x2e),
            .Selection = QColor(0x35, 0x35, 0x35),
            .Accent = QColor(0xd7, 0xa0, 0x42),
            .AccentHover = QColor(0xe2, 0xb2, 0x5e),
            .AccentPressed = QColor(0xc0, 0x8b, 0x33),
            .OnAccent = QColor(0x14, 0x14, 0x14),
            .TextSelection = QColor(0x6b, 0x51, 0x22),
            .TextOnSelection = Qt::white,
            .ToolBar = QColor(0x26, 0x26, 0x26),
            .ToolBarEdge = QColor(0x0e, 0x0e, 0x0e),
            .ToolBarGrip = QColor(0x13, 0x13, 0x13),
            .CaptionHover = QColor(0x2d, 0x2d, 0x2d),
            .CaptionPressed = QColor(0x29, 0x29, 0x29),
            .CaptionGlyphHover = Qt::white,
            .CloseGlyphHover = Qt::white,
            .CloseHover = QColor(0xc4, 0x2b, 0x1c),
            .ClosePressed = QColor(0xb2, 0x27, 0x1a),
            .Text = QColor(0xf0, 0xf0, 0xf0),
            .TextSecondary = QColor(0xc5, 0xc5, 0xc5),
            .TextPlaceholder = QColor(0x8a, 0x8a, 0x8a),
            .TextDisabled = QColor(0x78, 0x78, 0x78),
            .ScrollThumb = QColor(0x6a, 0x6a, 0x6a),
            .ScrollThumbHover = QColor(0x9e, 0x9e, 0x9e),
        };
    }

    static FluentTheme Light() {
        return {
            .Scheme = Qt::ColorScheme::Light,
            .Shell = QColor(0xee, 0xe8, 0xd5),
            .Pane = QColor(0xf8, 0xf2, 0xdf),
            .AlternateBase = QColor(0xf3, 0xf3, 0xf3),
            .Menu = QColor(0xfd, 0xf6, 0xe3),
            .MenuEdge = QColor(0xcc, 0xcc, 0xcc),
            .MenuHighlight = QColor(0xec, 0xe5, 0xd2),
            .MenuBarHover = QColor(0xe2, 0xdc, 0xc9),
            .Control = QColor(0xff, 0xfc, 0xf2),
            .ControlHover = QColor(0xf8, 0xf5, 0xeb),
            .ControlPressed = QColor(0xf2, 0xef, 0xe5),
            .ControlDisabled = QColor(0xf7, 0xf4, 0xea),
            .ControlBorder = QColor(0xd5, 0xd5, 0xd5),
            .ControlStroke = QColor(0xa0, 0xa0, 0xa0),
            .IndicatorFill = QColor(0xf9, 0xf9, 0xf9),
            .IndicatorBorder = QColor(0x86, 0x86, 0x86),
            .Border = QColor(0xd5, 0xd5, 0xd5),
            .SubtleBorder = QColor(0xe3, 0xe3, 0xe3),
            .BevelLight = QColor(0xff, 0xff, 0xff),
            .BevelMidlight = QColor(0xf0, 0xf0, 0xf0),
            .BevelDark = QColor(0xa0, 0xa0, 0xa0),
            .RowHover = QColor(0xe8, 0xe2, 0xcf),
            .TabHover = QColor(0xe6, 0xe6, 0xe6),
            .TabSelected = QColor(0xfa, 0xfa, 0xfa),
            .Selection = QColor(0xe1, 0xe1, 0xe1),
            .Accent = QColor(0xb5, 0x89, 0x00),
            .AccentHover = QColor(0xc3, 0x97, 0x0e),
            .AccentPressed = QColor(0x9f, 0x73, 0x00),
            .OnAccent = QColor(0x00, 0x2b, 0x36),
            .TextSelection = QColor(0xf0, 0xd9, 0xa8),
            .TextOnSelection = QColor(0x1a, 0x1a, 0x1a),
            .ToolBar = QColor(0xf5, 0xef, 0xdc),
            .ToolBarEdge = QColor(0xd0, 0xd0, 0xd0),
            .ToolBarGrip = QColor(0xb8, 0xb8, 0xb8),
            .CaptionHover = QColor(0xe5, 0xdf, 0xcc),
            .CaptionPressed = QColor(0xdc, 0xd6, 0xc3),
            .CaptionGlyphHover = QColor(0x1a, 0x1a, 0x1a),
            .CloseGlyphHover = Qt::white,
            .CloseHover = QColor(0xc4, 0x2b, 0x1c),
            .ClosePressed = QColor(0xb2, 0x27, 0x1a),
            .Text = QColor(0x07, 0x36, 0x42),
            .TextSecondary = QColor(0x58, 0x6e, 0x75),
            .TextPlaceholder = QColor(0x80, 0x80, 0x80),
            .TextDisabled = QColor(0xa0, 0xa0, 0xa0),
            .ScrollThumb = QColor(0x8a, 0x8a, 0x8a),
            .ScrollThumbHover = QColor(0x5c, 0x5c, 0x5c),
        };
    }

    static std::optional<FluentTheme> FromPluginStyle(const DeclaredStyle &style, Qt::ColorScheme scheme,
                                                      std::vector<std::string> &warnings);

    static FluentTheme ForScheme(Qt::ColorScheme scheme) {
        return scheme == Qt::ColorScheme::Light ? Light() : Dark();
    }
};
}
