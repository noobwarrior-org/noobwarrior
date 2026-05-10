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
// File: PlaceInfoCardWidget.cpp
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#include "PlaceInfoCardWidget.h"

using namespace NoobWarrior;

PlaceInfoCardWidget::PlaceInfoCardWidget(QWidget *parent) : QFrame(parent) {
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    InitWidgets();
}

void PlaceInfoCardWidget::Refresh(EmuDb* db, int64_t id) {
    db->PrepareStatement("SELECT Name, Description FROM Asset WHERE Id = ?;");

    mNoPlaceLabel->setVisible(false);
    mNoPlaceDescriptionLabel->setVisible(false);
}

void PlaceInfoCardWidget::InitWidgets() {
    mLayout = new QVBoxLayout(this);

    mNoPlaceLabel = new QLabel("No place selected");
    mNoPlaceLabel->setWordWrap(true);

    mNoPlaceDescriptionLabel = new QLabel("Select a place that you want to host by clicking on one of the places located within the sidebar beside me. Its information will show up here.");
    mNoPlaceDescriptionLabel->setWordWrap(true);

    mThumbnailLabel = new QLabel();

    mTitleLabel = new QLabel("Place");
    mTitleLabel->setWordWrap(true);

    QFont titleFont = mTitleLabel->font();
    titleFont.setPointSize(18);
    mTitleLabel->setFont(titleFont);
    mNoPlaceLabel->setFont(titleFont);

    mCreatorLabel = new QLabel("By No One!");
    mCreatorLabel->setWordWrap(true);

    QFont creatorFont = mCreatorLabel->font();
    creatorFont.setPointSize(16);
    mCreatorLabel->setFont(creatorFont);

    mDescriptionLabel = new QLabel("Description");
    mDescriptionLabel->setWordWrap(true);

    QFont descFont = mTitleLabel->font();
    descFont.setPointSize(12);
    mDescriptionLabel->setFont(descFont);
    mNoPlaceDescriptionLabel->setFont(descFont);

    mThumbnailLabel->setVisible(false);
    mTitleLabel->setVisible(false);
    mCreatorLabel->setVisible(false);
    mDescriptionLabel->setVisible(false);

    mLayout->addWidget(mNoPlaceLabel);
    mLayout->addWidget(mNoPlaceDescriptionLabel);

    mLayout->addWidget(mThumbnailLabel);
    mLayout->addWidget(mTitleLabel);
    mLayout->addWidget(mCreatorLabel);
    mLayout->addWidget(mDescriptionLabel);
    mLayout->addStretch();
}
