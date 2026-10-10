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
// File: FluentStyle.h
// Started by: Hattozo
// Started on: 10/9/2026
// Description: A cool fluent theme
#pragma once
#include "FluentTheme.h"
#include <QProxyStyle>
#include <QAbstractNativeEventFilter>

#include <memory>

namespace NoobWarrior {
class FluentStyle : public QProxyStyle {
public:
    explicit FluentStyle(FluentTheme theme = FluentTheme::Dark());
    ~FluentStyle() override;
    QPalette standardPalette() const override;
    void polish(QPalette &pal) override;
    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override;
    void drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &size, const QWidget *w) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
    int styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret) const override;
    QIcon standardIcon(StandardPixmap sp, const QStyleOption *opt, const QWidget *w) const override;
    const FluentTheme &Theme() const;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    std::unique_ptr<QAbstractNativeEventFilter> mFramelessFilter;
    FluentTheme mTheme;
};
}
