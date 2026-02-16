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
// File: HostServerDialog.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Dialog that allows for starting a Roblox game server
#include "HostServerDialog.h"
#include "../Application.h"

using namespace NoobWarrior;

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Host Server");
    InitWidgets();
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);

    mListWidget = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted, this);
    mMainLayout->addWidget(mListWidget);

    mStartServer = new QPushButton("Start Server");
    mMainLayout->addWidget(mStartServer);

    connect(mStartServer, &QPushButton::clicked, []() {
        gApp->LaunchClient({
            .NoobWarriorVersion = 1,
            .Type = ClientType::Server,
            .Hash = "07b64feec0bd47c1",
            .Version = "0.463.0.417004"
        });
    });
}