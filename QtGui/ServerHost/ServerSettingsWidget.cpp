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
// File: ServerSettingsWidget.cpp
// Started by: Hattozo
// Started on: 5/10/2026
// Description:
#include "ServerSettingsWidget.h"

using namespace NoobWarrior;

ServerSettingsWidget::ServerSettingsWidget(QWidget *parent) : QFrame(parent) {
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    InitWidgets();
}

void ServerSettingsWidget::InitWidgets() {
    mLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();

    mLayout->addLayout(mFormLayout);

    mFormLayout->addRow("Game Server Port", new QLineEdit("8080"));

    mInfoLabel = new QLabel(
        QString(
        "Open TCP port %1 and UDP port %2 on your router in order for the server to be accessible online. Make sure your friends join through port %3."
        ).arg(8080).arg(8081).arg(8080));
    mInfoLabel->setWordWrap(true);
    mLayout->addWidget(mInfoLabel);
}
