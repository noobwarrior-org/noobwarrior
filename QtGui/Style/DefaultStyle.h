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
// File: DefaultStyle.h
// Started by: Hattozo
// Started on: 7/11/2025
// Description: My style that I like for this program.
#pragma once
#include <QProxyStyle>

namespace NoobWarrior {
class DefaultStyle : public QProxyStyle {
public:
    DefaultStyle();
    void polish(QPalette &pal) override;
    void polish(QWidget *widget) override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
};
}
