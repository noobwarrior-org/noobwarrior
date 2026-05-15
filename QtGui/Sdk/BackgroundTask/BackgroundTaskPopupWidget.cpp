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
// File: BackgroundTaskPopupWidget.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The full dialog that pops up when you click on that little progress bar
#include "BackgroundTaskPopupWidget.h"

using namespace NoobWarrior;

BackgroundTaskPopupWidget::BackgroundTaskPopupWidget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    InitWidgets();
}

void BackgroundTaskPopupWidget::InitWidgets() {
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(outerLayout);

    mScrollArea = new QScrollArea(this);
    mWidget = new QWidget();
    mLayout = new QVBoxLayout(mWidget);
    mLayout->setAlignment(Qt::AlignTop);
    mWidget->setLayout(mLayout);
    mScrollArea->setWidget(mWidget);
    mScrollArea->setWidgetResizable(true);

    outerLayout->addWidget(mScrollArea);
    resize(QSize(300, 400));
}

void BackgroundTaskPopupWidget::AddTaskWidget(QWidget* w) {
    mLayout->addWidget(w);
}
