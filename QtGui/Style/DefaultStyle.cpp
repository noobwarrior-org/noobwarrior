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
#include <QDialog>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStyleOption>
#include <qnamespace.h>
#include <qstyle.h>

#if defined(Q_OS_WIN32)
#include <windows.h>
#endif

using namespace NoobWarrior;

DefaultStyle::DefaultStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {
}

void DefaultStyle::polish(QPalette &pal) {
    pal.setColor(QPalette::Window, QColor(60, 63, 65));

    pal.setColor(QPalette::Base,              QColor(60,63,65));
    pal.setColor(QPalette::AlternateBase, QColor(30, 32, 33));
    pal.setColor(QPalette::Button,            QColor(53, 53, 53));
    pal.setColor(QPalette::Link,              QColor(42, 130, 218));
    pal.setColor(QPalette::Highlight,         QColor(42, 130, 218));
    pal.setColor(QPalette::ToolTipBase,       QColor(71, 73, 74));
    pal.setColor(QPalette::BrightText, QColor(255, 255, 255));

    pal.setColor(QPalette::Light,       QColor(80, 81, 80));
    pal.setColor(QPalette::Midlight,       QColor(60, 63, 65));
    pal.setColor(QPalette::Dark,       QColor(30, 32, 33));
    pal.setColor(QPalette::Mid,       QColor(51,50,51));
    pal.setColor(QPalette::Shadow, QColor(10, 10, 10));

    pal.setColor(QPalette::WindowText, Qt::lightGray);
    pal.setColor(QPalette::PlaceholderText, Qt::gray);
    pal.setColor(QPalette::Text, Qt::lightGray);
    pal.setColor(QPalette::ButtonText, Qt::lightGray);
    pal.setColor(QPalette::ToolTipText, Qt::lightGray);

    QProxyStyle::polish(pal);
}

void DefaultStyle::polish(QWidget *widget) {
    auto *window = qobject_cast<QMainWindow*>(widget);
    if (window != nullptr) {
#if defined(Q_OS_WIN32)

#else
        // window->setWindowFlags(window->windowFlags() | Qt::FramelessWindowHint);
#endif
    }

#if !defined(Q_OS_MACOS)
    auto *menuBar = qobject_cast<QMenuBar*>(widget);
    if (menuBar != nullptr) {
        auto aestheticIcon = new QAction(QIcon(":/images/icon16_aa.png"), "");
        aestheticIcon->setShortcutVisibleInContextMenu(false);
        aestheticIcon->setShortcut(QKeySequence());
        aestheticIcon->setCheckable(false);
        aestheticIcon->setDisabled(true);
        aestheticIcon->setMenuRole(QAction::NoRole);
        aestheticIcon->setToolTip("me :D");
        if (menuBar->actions().size() > 0)
            menuBar->insertAction(menuBar->actions().at(0), aestheticIcon);
        else menuBar->addAction(aestheticIcon);

        auto windowTitleLabel = new QAction(menuBar->window()->windowTitle());
        windowTitleLabel->setCheckable(false);
        windowTitleLabel->setDisabled(true);
        menuBar->addAction(windowTitleLabel);
    }
#endif

    auto *menu = qobject_cast<QMenu*>(widget);
    if (menu != nullptr) {
        menu->setWindowFlags(menu->windowFlags() | Qt::NoDropShadowWindowHint);
    }

    auto *action = qobject_cast<QAction*>(widget);
    if (action != nullptr) {
    }

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

void DefaultStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const {
#if !defined(Q_OS_MACOS)
    switch (pe) {
    case QStyle::PE_PanelMenuBar:
    case QStyle::PE_PanelToolBar:
        return;
    default: break;
    }
    if (pe == PE_PanelMenuBar) {
        return;
    }
#endif
    QProxyStyle::drawPrimitive(pe, opt, p, w);
}

void DefaultStyle::drawControl(QStyle::ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w) const {
#if !defined(Q_OS_MACOS)
    if (ce == QStyle::CE_MenuBarItem) {
        auto menu_opt = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!menu_opt) return;

        QString text = menu_opt->text;
        if (text.startsWith("&"))
            text = text.slice(1);

        if (menu_opt->state & State_Selected) {
            QStyleOptionButton btn;
            btn.rect = menu_opt->rect;
            btn.state = menu_opt->state;
            drawPrimitive(
                QStyle::PE_PanelButtonCommand,
                &btn,
                p,
                w
            );
        }

        // p->fillRect(menu_opt->rect, menu_opt->state & State_Selected ? menu_opt->palette.highlight().color() : menu_opt->palette.window().color());
        drawItemPixmap(p, menu_opt->rect, Qt::AlignVCenter | Qt::AlignHCenter, menu_opt->icon.pixmap(16, 16));
        drawItemText(p, menu_opt->rect.adjusted(0, 0, 0, 0), Qt::AlignVCenter | Qt::AlignHCenter, menu_opt->palette, menu_opt->state & State_Enabled, text, QPalette::Text);
        return;
    }
#endif
    QProxyStyle::drawControl(ce, opt, p, w);
}

int DefaultStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const {
    switch (metric) {
    default: return QProxyStyle::pixelMetric(metric, option, widget);
    case QStyle::PM_MessageBoxIconSize: return 32;
#if !defined(Q_OS_MACOS)
    case QStyle::PM_MenuBarVMargin: return 4;
#endif
    }
}