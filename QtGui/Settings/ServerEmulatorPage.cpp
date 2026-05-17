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

    mHttpsPortInput = new QLineEdit();
    mHttpsPortInput->setValidator(new QIntValidator(0, 65535, this));
    mForm->addRow("HTTPS Port", mHttpsPortInput);

    mHttpPortInput = new QLineEdit();
    mHttpPortInput->setValidator(new QIntValidator(0, 65535, this));
    mForm->addRow("HTTP Port", mHttpPortInput);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mForm->addRow(line);

    mBackupModeInput = new QCheckBox();
    mForm->addRow("Backup Mode", mBackupModeInput);

    mBackupDbDropdown = new QComboBox();
    mForm->addRow("Backup to Database", mBackupDbDropdown);

    auto *backupModeInfo = new QLabel("When turned on, Backup Mode makes the server emulator automatically back up assets retrieved from Roblox into a database of your choice.");
    backupModeInfo->setWordWrap(true);
    mForm->addRow(backupModeInfo);

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
    mHttpsPortInput->setText(QString::number(reg->GetKeyValue<uint16_t>("emu.port_https").value_or(53640)));
    mHttpPortInput->setText(QString::number(reg->GetKeyValue<uint16_t>("emu.port_http").value_or(8080)));
    mBackupModeInput->setChecked(reg->GetKeyValue<bool>("emu.backup_mode").value_or(false));
}

void ServerEmulatorPage::Serialize(Registry* reg) {
    uint16_t oldHttpsPort = reg->GetKeyValue<uint16_t>("emu.port_https").value_or(53640);
    reg->SetKeyValue<uint16_t>("emu.port_https", mHttpsPortInput->text().toUShort());

    uint16_t oldHttpPort = reg->GetKeyValue<uint16_t>("emu.port_http").value_or(8080);
    reg->SetKeyValue<uint16_t>("emu.port_http", mHttpPortInput->text().toUShort());

    if (oldHttpsPort != mHttpsPortInput->text().toUShort() || oldHttpPort != mHttpPortInput->text().toUShort()) {
        gApp->GetCore()->RestartServerEmulator();
    }

    reg->SetKeyValue<bool>("emu.backup_mode", mBackupModeInput->isChecked());
}
