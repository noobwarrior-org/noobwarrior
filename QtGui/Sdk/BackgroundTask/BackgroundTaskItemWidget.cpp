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
// File: BackgroundTaskItemWidget.cpp
// Started by: Hattozo
// Started on: 5/14/2026
// Description:
#include "BackgroundTaskItemWidget.h"

using namespace NoobWarrior;

BackgroundTaskItemWidget::BackgroundTaskItemWidget(QWidget *parent) : QWidget(parent) {
    mLayout = new QVBoxLayout(this);

    mTitle = new QLabel(this);
    QFont titleFont = mTitle->font();
    titleFont.setPointSize(16);
    mTitle->setFont(titleFont);

    mCaption = new QLabel(this);

    mProgressBar = new QProgressBar(this);
    mProgressBar->setRange(0, 100);
    
    mLayout->addWidget(mTitle);
    mLayout->addWidget(mCaption);
    mLayout->addWidget(mProgressBar);
}

void BackgroundTaskItemWidget::OnUpdate(double progress, const QString &title, const QString &caption) {
    mTitle->setText(title);
    mCaption->setText(caption);
    mProgressBar->setValue(static_cast<int>(progress * 100));
}