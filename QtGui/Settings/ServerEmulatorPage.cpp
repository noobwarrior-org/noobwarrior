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
#include "Sdk/EmuDbComboBox.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QWidget>

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

    mAssetGrabInput = new QCheckBox();
    mForm->addRow("Asset Grab Mode", mAssetGrabInput);

    mSaveDbDropdown = new EmuDbComboBox(EmuDbComboBox::Mode::ShowMounted);
    mForm->addRow("Save to Database", mSaveDbDropdown);

    auto *backupModeInfo = new QLabel("When enabled, any asset that is retrieved from Roblox services will be downloaded and saved to a database of your choice.");
    backupModeInfo->setWordWrap(true);
    mForm->addRow(backupModeInfo);

    // --- Authentication (hosting) ---
    QFrame* authLine = new QFrame();
    authLine->setFrameShape(QFrame::HLine);
    authLine->setFrameShadow(QFrame::Sunken);
    mForm->addRow(authLine);
    mForm->addRow(new QLabel("<b>Authentication</b>"));

    mAuthEnabledInput = new QCheckBox();
    mForm->addRow("Require authentication to join", mAuthEnabledInput);

    mAuthTypeInput = new QComboBox();
    mAuthTypeInput->addItem("Master (I authenticate players)", "master");
    mAuthTypeInput->addItem("Slave (a master server authenticates players)", "slave");
    mForm->addRow("Authentication mode", mAuthTypeInput);

    mAuthMasterUrlInput = new QLineEdit();
    mAuthMasterUrlInput->setPlaceholderText("https://master.example.com");
    mForm->addRow("Master server URL", mAuthMasterUrlInput);

    mAuthFederatedLoginInput = new QCheckBox();
    mForm->addRow("Allow logins from other federated masters", mAuthFederatedLoginInput);

    mAuthAllowGuestsInput = new QCheckBox();
    mForm->addRow("Allow guests", mAuthAllowGuestsInput);

    mAuthPasswordBasedInput = new QCheckBox();
    mForm->addRow("Password-based login", mAuthPasswordBasedInput);

    mAuthAllowRegistrationInput = new QCheckBox();
    mForm->addRow("Allow account registration", mAuthAllowRegistrationInput);

    mAuthTicketTtlInput = new QLineEdit();
    mAuthTicketTtlInput->setValidator(new QIntValidator(1, 86400, this));
    mForm->addRow("Join ticket lifetime (seconds)", mAuthTicketTtlInput);

    connect(mAuthTypeInput, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        UpdateAuthSlaveFieldState();
    });

    // The page is long, so keep it scrollable below the fixed title/description.
    auto* content = new QWidget();
    content->setLayout(mForm);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    Layout->addWidget(scroll);
}

void ServerEmulatorPage::UpdateAuthSlaveFieldState() {
    bool slave = mAuthTypeInput->currentData().toString() == "slave";
    mAuthMasterUrlInput->setEnabled(slave);
    mAuthFederatedLoginInput->setEnabled(slave);
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
    mHttpsPortInput->setText(QString::number(reg->GetKeyValue<uint16_t>("emu.https_port").value_or(53640)));
    mHttpPortInput->setText(QString::number(reg->GetKeyValue<uint16_t>("emu.http_port").value_or(8080)));
    mAssetGrabInput->setChecked(reg->GetKeyValue<bool>("emu.asset_grab_mode").value_or(false));
    auto dbFilePath = reg->GetKeyValue<std::string>("emu.asset_grab_db");
    if (dbFilePath.has_value()) {
        EmuDb* db = gApp->GetCore()->GetEmuDbManager()->GetDbFromFilePath(dbFilePath.value());
        if (db != nullptr)
            mSaveDbDropdown->SetDatabase(db);
    }

    mAuthEnabledInput->setChecked(reg->GetKeyValue<bool>("emu.auth.enabled").value_or(false));
    std::string authType = reg->GetKeyValue<std::string>("emu.auth.type").value_or("master");
    mAuthTypeInput->setCurrentIndex(authType == "slave" ? 1 : 0);
    mAuthMasterUrlInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("emu.auth.master").value_or("")));
    mAuthFederatedLoginInput->setChecked(reg->GetKeyValue<bool>("emu.auth.federated_login").value_or(true));
    mAuthAllowGuestsInput->setChecked(reg->GetKeyValue<bool>("emu.auth.allow_guests").value_or(false));
    mAuthPasswordBasedInput->setChecked(reg->GetKeyValue<bool>("emu.auth.password_based").value_or(true));
    mAuthAllowRegistrationInput->setChecked(reg->GetKeyValue<bool>("emu.auth.allow_registration").value_or(false));
    mAuthTicketTtlInput->setText(QString::number(reg->GetKeyValue<int64_t>("emu.auth.ticket_ttl").value_or(120)));
    UpdateAuthSlaveFieldState();
}

void ServerEmulatorPage::Serialize(Registry* reg) {
    uint16_t oldHttpsPort = reg->GetKeyValue<uint16_t>("emu.https_port").value_or(53640);
    reg->SetKeyValue<uint16_t>("emu.https_port", mHttpsPortInput->text().toUShort());

    uint16_t oldHttpPort = reg->GetKeyValue<uint16_t>("emu.http_port").value_or(8080);
    reg->SetKeyValue<uint16_t>("emu.http_port", mHttpPortInput->text().toUShort());

    if (oldHttpsPort != mHttpsPortInput->text().toUShort() || oldHttpPort != mHttpPortInput->text().toUShort()) {
        gApp->GetCore()->RestartServerEmulator();
    }

    reg->SetKeyValue<bool>("emu.asset_grab_mode", mAssetGrabInput->isChecked());
    EmuDb* db = mSaveDbDropdown->GetSelectedDatabase();
    if (db != nullptr) {
        if (mAssetGrabInput->isChecked() && !db->AllowsRuntimeWrites()) {
            EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
            const EmuDbManager::MountInfo* info = manager->GetMountInfo(db);
            QString reason = info != nullptr && !info->OwnerPluginId.empty()
                ? QString("it is provided read-only by the plugin \"%1\"")
                      .arg(QString::fromStdString(info->OwnerPluginId))
                : QString("its Mutable setting is turned off");
            QMessageBox::warning(this, "Asset Grab Mode",
                QString("\"%1\" cannot be used as the Asset Grab Mode target because %2.\n\n"
                        "Nothing would be saved. Pick a different database.")
                    .arg(QString::fromStdString(db->GetTitle().empty() ? db->GetFileName() : db->GetTitle()))
                    .arg(reason));
        }
        reg->SetKeyValue<std::string>("emu.asset_grab_db", db->GetFilePath().string());
    }

    reg->SetKeyValue<bool>("emu.auth.enabled", mAuthEnabledInput->isChecked());
    reg->SetKeyValue<std::string>("emu.auth.type", mAuthTypeInput->currentData().toString().toStdString());
    reg->SetKeyValue<std::string>("emu.auth.master", mAuthMasterUrlInput->text().trimmed().toStdString());
    reg->SetKeyValue<bool>("emu.auth.federated_login", mAuthFederatedLoginInput->isChecked());
    reg->SetKeyValue<bool>("emu.auth.allow_guests", mAuthAllowGuestsInput->isChecked());
    reg->SetKeyValue<bool>("emu.auth.password_based", mAuthPasswordBasedInput->isChecked());
    reg->SetKeyValue<bool>("emu.auth.allow_registration", mAuthAllowRegistrationInput->isChecked());
    if (!mAuthTicketTtlInput->text().isEmpty())
        reg->SetKeyValue<int64_t>("emu.auth.ticket_ttl", mAuthTicketTtlInput->text().toLongLong());
}
