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
// File: DarculaTheme.h
// Started by: Hattozo
// Started on: 10/10/2026
// Description: The set of colors DarculaStyle paints with
#pragma once
#include <QColor>

#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
struct DeclaredStyle;

struct DarculaTheme {
    static constexpr const char *kBaseName = "darcula";

    Qt::ColorScheme Scheme;
    QColor Window;
    QColor Base;
    QColor AlternateBase;
    QColor Button;
    QColor Link;
    QColor Highlight;
    QColor ToolTipBase;
    QColor BrightText;
    QColor Light;
    QColor Midlight;
    QColor Dark;
    QColor Mid;
    QColor Shadow;
    QColor Text;
    QColor PlaceholderText;
    QColor TabPane;
    QColor ListBase;

    static DarculaTheme DarkPreset() {
        return {
            .Scheme = Qt::ColorScheme::Dark,
            .Window = QColor(60, 63, 65),
            .Base = QColor(60, 63, 65),
            .AlternateBase = QColor(30, 32, 33),
            .Button = QColor(53, 53, 53),
            .Link = QColor(42, 130, 218),
            .Highlight = QColor(42, 130, 218),
            .ToolTipBase = QColor(71, 73, 74),
            .BrightText = QColor(255, 255, 255),
            .Light = QColor(80, 81, 80),
            .Midlight = QColor(60, 63, 65),
            .Dark = QColor(30, 32, 33),
            .Mid = QColor(51, 50, 51),
            .Shadow = QColor(10, 10, 10),
            .Text = QColor(Qt::lightGray),
            .PlaceholderText = QColor(Qt::gray),
            .TabPane = QColor(43, 42, 43),
            .ListBase = QColor(43, 42, 43),
        };
    }

    static DarculaTheme LightPreset() {
        return {
            .Scheme = Qt::ColorScheme::Light,
            .Window = QColor(242, 242, 242),
            .Base = QColor(255, 255, 255),
            .AlternateBase = QColor(245, 245, 245),
            .Button = QColor(250, 250, 250),
            .Link = QColor(32, 104, 214),
            .Highlight = QColor(38, 117, 191),
            .ToolTipBase = QColor(247, 247, 247),
            .BrightText = QColor(255, 255, 255),
            .Light = QColor(255, 255, 255),
            .Midlight = QColor(242, 242, 242),
            .Dark = QColor(160, 160, 160),
            .Mid = QColor(200, 200, 200),
            .Shadow = QColor(120, 120, 120),
            .Text = QColor(30, 30, 30),
            .PlaceholderText = QColor(128, 128, 128),
            .TabPane = QColor(250, 250, 250),
            .ListBase = QColor(250, 250, 250),
        };
    }

    static DarculaTheme ForScheme(Qt::ColorScheme scheme) {
        return scheme == Qt::ColorScheme::Light ? LightPreset() : DarkPreset();
    }

    static std::optional<DarculaTheme> FromPluginStyle(const DeclaredStyle &style, Qt::ColorScheme scheme,
                                                       std::vector<std::string> &warnings);
};
}
