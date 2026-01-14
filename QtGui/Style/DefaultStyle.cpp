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
// File: DefaultStyle.cpp
// Started by: Hattozo
// Started on: 7/11/2025
// Description: My style that I like for this program.
#include "DefaultStyle.h"

#include <NoobWarrior/Log.h>

#include <QApplication>
#include <QFont>
#include <QStyleFactory>
#include <QPainter>
#include <QWidget>
#include <QFrame>
#include <QTabWidget>
#include <QTabBar>

NoobWarrior::DefaultStyle::DefaultStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {
}

void NoobWarrior::DefaultStyle::polish(QPalette &pal) {
    pal.setColor(QPalette::Window, QColor(60, 63, 65));

    pal.setColor(QPalette::Base,              QColor(60,63,65));
    pal.setColor(QPalette::Button,            QColor(53, 53, 53));
    pal.setColor(QPalette::Link,              QColor(42, 130, 218));
    pal.setColor(QPalette::Highlight,         QColor(42, 130, 218));
    pal.setColor(QPalette::ToolTipBase,       QColor(71, 73, 74));

    pal.setColor(QPalette::Light,       QColor(80, 81, 80));
    pal.setColor(QPalette::Midlight,       QColor(80, 81, 80));
    pal.setColor(QPalette::Dark,       QColor(51,50,51));
    pal.setColor(QPalette::Mid,       QColor(51,50,51));

    pal.setColor(QPalette::WindowText, Qt::lightGray);
    pal.setColor(QPalette::Text, Qt::lightGray);
    pal.setColor(QPalette::ButtonText, Qt::lightGray);
    pal.setColor(QPalette::ToolTipText, Qt::lightGray);
    QProxyStyle::polish(pal);
}

void NoobWarrior::DefaultStyle::polish(QWidget *widget) {
    QFont font = widget->font();
    font.setFamily("Source Sans Pro");

    if (widget->palette() == QProxyStyle::standardPalette()) {
        widget->setPalette(QApplication::palette());
    }

    auto *frame = qobject_cast<QFrame*>(widget);
    if (frame != nullptr) {
        QPalette framePalette = QApplication::palette();
        framePalette.setColor(QPalette::Window, framePalette.color(QPalette::Window).darker(150));
        frame->setPalette(framePalette);
    }

    // darken background of qtabwidget
    auto *tabWidget = qobject_cast<QTabWidget*>(widget->parentWidget());
    if (tabWidget != nullptr && qobject_cast<QTabBar*>(widget) == nullptr) {
        widget->setAutoFillBackground(true);
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, QColor(43, 42, 43));
        // palette.setColor(widget->backgroundRole(), palette.color(widget->backgroundRole()).darker(128));
        widget->setPalette(palette);
    }

    QProxyStyle::polish(widget);
}
