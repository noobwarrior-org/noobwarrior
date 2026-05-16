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
// File: ServerEmulatorPage.cpp
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#include "ServerEmulatorPage.h"
#include "Application.h"

#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

ServerEmulatorPage::ServerEmulatorPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void ServerEmulatorPage::InitWidgets() {
    mForm = new QFormLayout();

    mPortInput = new QLineEdit();
    mPortInput->setValidator(new QIntValidator(0, 65535, this));
    mForm->addRow("Port", mPortInput);

    Layout->addLayout(mForm);
}

const QString ServerEmulatorPage::GetTitle() {
    return "Server Emulator";
}

const QString ServerEmulatorPage::GetDescription() {
    return "Used for modifying the state of the server emulator that noobWarrior runs in the background. If you don't know what that is, it basically emulates the Roblox website.";
}

const QIcon ServerEmulatorPage::GetIcon() {
    return QIcon(":/images/silk/drive_web.png");
}

void ServerEmulatorPage::Deserialize(Registry* reg) {
    mPortInput->setText(QString::number(reg->GetKeyValue<uint16_t>("emu.port").value_or(53640)));
}

void ServerEmulatorPage::Serialize(Registry* reg) {
    uint16_t oldPort = reg->GetKeyValue<uint16_t>("emu.port").value_or(53640);
    reg->SetKeyValue<uint16_t>("emu.port", mPortInput->text().toUShort());
    if (oldPort != mPortInput->text().toUShort()) {
        gApp->GetCore()->RestartServerEmulator();
    }
}
