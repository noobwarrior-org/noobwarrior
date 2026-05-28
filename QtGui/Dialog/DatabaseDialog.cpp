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
// File: DatabaseDialog.cpp
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#include "DatabaseDialog.h"
#include "Application.h"

using namespace NoobWarrior;

DatabaseDialog::DatabaseDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Databases");
    InitWidgets();
}

void DatabaseDialog::InitWidgets() {
    mGridLayout = new QGridLayout(this);

    mAvailableFrame = new QFrame();
    mAvailableLayout = new QVBoxLayout(mAvailableFrame);
    mAvailableLabel = new QLabel("Available");
    mAvailableList = new EmuDbListWidget(EmuDbListWidget::Mode::ShowNotMounted);

    mAvailableLayout->addWidget(mAvailableLabel);
    mAvailableLayout->addWidget(mAvailableList);

    mAvailableFrame->setAutoFillBackground(true);

    mSelectedFrame = new QFrame();
    mSelectedLayout = new QVBoxLayout(mSelectedFrame);
    mSelectedLabel = new QLabel("Selected");
    mSelectedList = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);

    mSelectedLayout->addWidget(mSelectedLabel);
    mSelectedLayout->addWidget(mSelectedList);

    mSelectedFrame->setAutoFillBackground(true);

    mSelectorArrowFrame = new QFrame();
    mSelectorArrowLayout = new QVBoxLayout(mSelectorArrowFrame);
    mSelectorArrow_MoveOneRight = new QPushButton(">");
    mSelectorArrow_MoveAllRight = new QPushButton(">>");
    mSelectorArrow_MoveOneLeft = new QPushButton("<");
    mSelectorArrow_MoveAllLeft = new QPushButton("<<");

    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveOneRight);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveAllRight);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveOneLeft);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveAllLeft);

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectorArrowFrame, 0, 1);
    mGridLayout->addWidget(mSelectedFrame, 0, 2);
}
