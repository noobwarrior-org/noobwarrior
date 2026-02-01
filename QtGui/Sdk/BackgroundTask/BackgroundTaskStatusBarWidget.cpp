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
// File: BackgroundTaskStatusBarWidget.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The little progress bar that you see in your status bar
#include "BackgroundTaskStatusBarWidget.h"
#include "BackgroundTaskPopupWidget.h"

#include <QApplication>
#include <QStyle>
#include <QMouseEvent>

using namespace NoobWarrior;

BackgroundTaskStatusBarWidget::BackgroundTaskStatusBarWidget(QWidget *parent) : QWidget(parent),
    mLayout(this),
    mPopupWidget(nullptr)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mLayout.setSizeConstraint(QLayout::SetMaximumSize);
    
    mLayout.addWidget(&mLabel);
    mLayout.addWidget(&mProgressBar);

    auto bellLabel = new QLabel();
    QIcon bellIcon = QIcon(":/images/silk/bell.png");
    bellLabel->setPixmap(bellIcon.pixmap(QSize(16, 16)));
    mLayout.addWidget(bellLabel);

    mLabel.setVisible(false);
    mProgressBar.setVisible(false);
}

void BackgroundTaskStatusBarWidget::SetPopupWidget(BackgroundTaskPopupWidget *widget) {
    mPopupWidget = widget;
}

void BackgroundTaskStatusBarWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (mPopupWidget != nullptr)
        mPopupWidget->isVisible() ? mPopupWidget->hide() : mPopupWidget->show();
}
