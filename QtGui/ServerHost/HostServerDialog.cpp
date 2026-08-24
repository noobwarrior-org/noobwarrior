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
// Description: Dialog that allows for starting a game server
#include "HostServerDialog.h"
#include "../Application.h"
#include "../OnlineWindow/MasterHttp.h"
#include "../OnlineWindow/MasterLoginDialog.h"
#include "../OnlineWindow/MasterServerStore.h"
#include "PlaceInfoCardWidget.h"
#include "Sdk/Item/UniverseTreeWidget.h"

#include <nlohmann/json.hpp>

#include <QLabel>
#include <QMessageBox>
#include <qsizepolicy.h>

using namespace NoobWarrior;

Q_DECLARE_METATYPE(Engine)

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Host Server");
    InitWidgets();
    resize(QSize(size().width() + 192, size().height() + 224));
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);
    mLayout = new QVBoxLayout();

    mDbListWidget = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);
    mUniverseTreeWidget = new UniverseTreeWidget(this);

    mMainLayout->addWidget(mDbListWidget);
    mMainLayout->addWidget(mUniverseTreeWidget);

    connect(mDbListWidget, &QListWidget::currentItemChanged, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        mUniverseTreeWidget->Populate(db);
    });
    mDbListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mDbListWidget->setCurrentRow(0);

    mPlaceInfoCardWidget = new PlaceInfoCardWidget();

    connect(mUniverseTreeWidget, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (!current) {
            mPlaceInfoCardWidget->Refresh(nullptr, 0);
            return;
        }
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        mPlaceInfoCardWidget->Refresh(db, current->data(0, Qt::UserRole).toLongLong());
    });

    mServerSettingsFrame = new QFrame();
    mServerSettingsFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    mServerSettingsLayout = new QVBoxLayout(mServerSettingsFrame);
    mServerSettingsFormLayout = new QFormLayout();

    mServerSettingsLayout->addLayout(mServerSettingsFormLayout);

    mPortInput = new QLineEdit("53640");
    mServerSettingsFormLayout->addRow("Game Server Port", mPortInput);

    mMasterServerBox = new QComboBox();
    mMasterServerBox->addItem("None", QString());
    for (const MasterServerEntry &entry : MasterServerStore::Load())
        mMasterServerBox->addItem(entry.Name, entry.Url);
    mServerSettingsFormLayout->addRow("Master Server", mMasterServerBox);

    {
        auto *reg = gApp->GetCore()->GetRegistry();
        QString preselect;
        if (reg->GetKeyValue<std::string>("emu.auth.type").value_or("master") == "slave") {
            preselect = MasterServerStore::NormalizeUrl(QString::fromStdString(
                reg->GetKeyValue<std::string>("emu.auth.master").value_or("")));
        }
        if (preselect.isEmpty()) {
            if (Account *active = gApp->GetCore()->GetMasterKeychain()->GetActiveAccount();
                active != nullptr)
                preselect = MasterServerStore::NormalizeUrl(QString::fromStdString(active->Url));
        }
        if (!preselect.isEmpty()) {
            int index = mMasterServerBox->findData(preselect);
            if (index >= 0)
                mMasterServerBox->setCurrentIndex(index);
        }
    }

    mPublicInput = new QCheckBox();
    if (!mMasterServerBox->currentData().toString().isEmpty()) {
        mPublicInput->setChecked(
            gApp->GetCore()->GetRegistry()->GetKeyValue<bool>("emu.master.announce").value_or(false));
    }
    mServerSettingsFormLayout->addRow("Public", mPublicInput);

    // A master may only list servers hosted by its own accounts. Asking it up front beats letting
    // the user tick Public, start a server, and find out from a log line that it was never listed.
    mPublicHintLabel = new QLabel();
    mPublicHintLabel->setWordWrap(true);
    mPublicHintLabel->setVisible(false);
    mServerSettingsFormLayout->addRow("", mPublicHintLabel);

    mMasterSignInButton = new QPushButton("Sign in to this master server...");
    mMasterSignInButton->setVisible(false);
    connect(mMasterSignInButton, &QPushButton::clicked, this, &HostServerDialog::PromptMasterSignIn);
    mServerSettingsFormLayout->addRow("", mMasterSignInButton);

    connect(mMasterServerBox, &QComboBox::currentIndexChanged, this, [this]() {
        RefreshMasterHostingState();
    });
    RefreshMasterHostingState();

    mServerSettingsInfoLabel = new QLabel();
    mServerSettingsInfoLabel->setWordWrap(true);
    mServerSettingsInfoLabel->setStyleSheet("QLabel { color: yellow; }");
    auto updateText = [this]() {
        auto* registry = gApp->GetCore()->GetRegistry();
        const auto emu_https_port = registry->GetKeyValue<uint16_t>(
            "emu.https_port").value_or(53640);
        mServerSettingsInfoLabel->setText(
            QString(
            "Open TCP port %1 and UDP port %2 on your router in order for the server to be accessible online. Make sure your friends join through port %3."
            ).arg(emu_https_port).arg(mPortInput->text()).arg(emu_https_port)
        );
    };
    updateText();
    connect(mPortInput, &QLineEdit::textChanged, [updateText]() {
        updateText();
    });
    mServerSettingsLayout->addWidget(mServerSettingsInfoLabel);

    auto* registry = gApp->GetCore()->GetRegistry();
    const int turnPort = registry->GetKeyValue<int>(
        "emu.voice_turn.port").value_or(3478);
    const int relayPortBegin = registry->GetKeyValue<int>(
        "emu.voice_turn.relay_port_begin").value_or(49160);
    const int relayPortEnd = registry->GetKeyValue<int>(
        "emu.voice_turn.relay_port_end").value_or(49200);
    mServerSettingsVcInfoLabel = new QLabel();
    mServerSettingsVcInfoLabel->setWordWrap(true);
    mServerSettingsVcInfoLabel->setStyleSheet("QLabel { color: orange; }");
    mServerSettingsVcInfoLabel->setText(
        QString(
        "You have selected a game with voice chat. This feature will not work online unless if you have opened UDP port %1 and UDP ports %2-%3 on your router."
        ).arg(turnPort).arg(relayPortBegin).arg(relayPortEnd)
    );
    mServerSettingsVcInfoLabel->setVisible(false);
    mServerSettingsLayout->addWidget(mServerSettingsVcInfoLabel);
    connect(mUniverseTreeWidget, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        mServerSettingsVcInfoLabel->setVisible(false);
        if (!current)
            return;

        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        if (db == nullptr)
            return;

        std::optional<int64_t> universeId = db->GetUniverseIdForPlace(current->data(0, Qt::UserRole).toLongLong());
        if (!universeId)
            return;

        mServerSettingsVcInfoLabel->setVisible(db->GetUniverseVoiceChatEnabled(*universeId).value_or(false));
    });

    auto* engineRow = new QHBoxLayout();
    engineRow->addWidget(new QLabel("Host engine:"));
    mEngineCombo = new QComboBox();
    int engines = 0;
    for (const Engine &engine : gApp->GetCore()->GetInstalledEngines()) {
        engines++;
        if (engine.Side == EngineSide::Server || (engine.Side == EngineSide::Studio && ParseEraVersion(engine.Version) >= 574)) {
            mEngineCombo->addItem(QString("%1 - %2").arg(engine.Side == EngineSide::Server ? "RCCService" : "Studio").arg(engine.Version),
            QVariant::fromValue(engine));
        }
    }
    if (engines <= 0) { // not sure why this would ever be less than zero but whatever
        mEngineCombo->setPlaceholderText("No engines found");
        mEngineCombo->setCurrentIndex(-1); // do this so that the placeholder "No engines found" text shows
        mEngineCombo->setDisabled(true);
    }
    engineRow->addWidget(mEngineCombo, 1);

    mButtonBox = new QDialogButtonBox();

    mStartServer = mButtonBox->addButton("Start Server", QDialogButtonBox::AcceptRole);
    mCloseButton = mButtonBox->addButton("Close", QDialogButtonBox::RejectRole);

    mLayout->addWidget(mPlaceInfoCardWidget);
    mLayout->addWidget(mServerSettingsFrame);
    mLayout->addLayout(engineRow);
    mLayout->addWidget(mButtonBox);
    mMainLayout->addLayout(mLayout);

    connect(mStartServer, &QPushButton::clicked, [this]() {
        std::optional<int64_t> placeId = mUniverseTreeWidget->GetSelectedPlaceId();

        QList<QTreeWidgetItem*> items = mUniverseTreeWidget->selectedItems();
        if (items.size() <= 0) {
            QMessageBox::critical(this, "Error", "You need to select a place!");
            return;
        }

        QTreeWidgetItem* item = items.at(0);

        int flagThatTellsUsIfSomethingWentWrong = item->data(0, Qt::UserRole + 1).toInt();
        if (flagThatTellsUsIfSomethingWentWrong != 0) {
            QMessageBox::critical(this, "Error", flagThatTellsUsIfSomethingWentWrong == 1 ? "This universe has no places!" : "This universe does not have a set start place!");
            return;
        }

        if (!placeId.has_value()) {
            QMessageBox::critical(this, "Error", "No selected place id!");
            return;
        }

        if (!mDbListWidget->GetSelectedDatabase()->DoesItemExist(ItemType::Asset, placeId.value())) {
            QMessageBox::critical(this, "Error", "This place no longer exists in the database!");
            return;
        }

        if (mEngineCombo->currentData().canConvert<Engine>()) {
            ApplyMasterServerSelection();

            const auto engine = mEngineCombo->currentData().value<Engine>();
            bool portOk = false;
            uint16_t gamePort = static_cast<uint16_t>(mPortInput->text().toUInt(&portOk));
            if (!portOk || gamePort == 0) {
                QMessageBox::critical(this, "Error", "Please enter a valid game server port!");
                return;
            }
            gApp->LaunchEngine({
                .Engine = engine,
                .Port = gamePort,
                .PlaceId = placeId.value(),
                .LaunchSide = EngineSide::Server
            });
        } else {
            QMessageBox::critical(this, "Error", "You are trying to start a server without a selected engine.");
        }
        
        close();
    });

    connect(mCloseButton, &QPushButton::clicked, [this]() {
        close();
    });
}

void HostServerDialog::RefreshMasterHostingState() {
    QString masterUrl = mMasterServerBox->currentData().toString();

    // "None" means nothing is announced anywhere, so Public has nothing to mean.
    if (masterUrl.isEmpty()) {
        mPublicInput->setChecked(false);
        mPublicInput->setEnabled(false);
        mPublicHintLabel->setVisible(false);
        mMasterSignInButton->setVisible(false);
        return;
    }

    // Already signed in to it, so it will take our announce whatever its policy is.
    if (MasterHttp::IsSignedIn(masterUrl)) {
        mPublicInput->setEnabled(true);
        mPublicHintLabel->setVisible(false);
        mMasterSignInButton->setVisible(false);
        return;
    }

    // Signed out, ask before letting the user commit to something the master will refuse.
    mPublicInput->setEnabled(false);
    mMasterSignInButton->setVisible(false);
    mPublicHintLabel->setStyleSheet(QString());
    mPublicHintLabel->setText("Checking whether this master server accepts servers from signed-out hosts...");
    mPublicHintLabel->setVisible(true);

    QString pending = masterUrl;
    MasterHttp::Get(this, masterUrl, "/fed/v1/info", [this, pending](const MasterResponse &response) {
        // The user may have picked a different master while this was in flight.
        if (mMasterServerBox->currentData().toString() != pending)
            return;

        if (!response.Ok) {
            ApplyMasterHostingState(false, false);
            return;
        }

        nlohmann::json info = nlohmann::json::parse(response.Body, nullptr, false);
        if (info.is_discarded() || !info.is_object()) {
            ApplyMasterHostingState(false, false);
            return;
        }

        ApplyMasterHostingState(true, info.value("RequiresAccountToHost", false));
    });
}

void HostServerDialog::ApplyMasterHostingState(bool reachable, bool requiresAccount) {
    if (!reachable) {
        mPublicInput->setEnabled(true);
        mPublicHintLabel->setStyleSheet("QLabel { color: orange; }");
        mPublicHintLabel->setText("Couldn't reach that master server to check its policy. You can still "
                                  "try to make this server public, but it may not get listed.");
        mPublicHintLabel->setVisible(true);
        mMasterSignInButton->setVisible(false);
        return;
    }

    if (!requiresAccount) {
        mPublicInput->setEnabled(true);
        mPublicHintLabel->setVisible(false);
        mMasterSignInButton->setVisible(false);
        return;
    }

    // It would refuse us. Say so where the decision is being made, and offer the fix inline.
    mPublicInput->setChecked(false);
    mPublicInput->setEnabled(false);
    mPublicHintLabel->setStyleSheet("QLabel { color: orange; }");
    mPublicHintLabel->setText("This master server only lists servers hosted by its own accounts, and you "
                              "aren't signed in to it. Sign in to make this server public.");
    mPublicHintLabel->setVisible(true);
    mMasterSignInButton->setVisible(true);
}

void HostServerDialog::PromptMasterSignIn() {
    QString masterUrl = mMasterServerBox->currentData().toString();
    if (masterUrl.isEmpty())
        return;

    MasterLoginDialog dialog(this, "Sign in to host publicly",
                             "Signing in lets this master server list the server you are about to start.",
                             masterUrl, false);
    if (dialog.exec() != QDialog::Accepted)
        return;

    mMasterSignInButton->setEnabled(false);
    mPublicHintLabel->setStyleSheet(QString());
    mPublicHintLabel->setText("Signing in...");

    MasterHttp::SignIn(this, dialog.MasterUrl(), dialog.Username(), dialog.Password(), [this](bool ok) {
        mMasterSignInButton->setEnabled(true);
        if (!ok) {
            mPublicHintLabel->setStyleSheet("QLabel { color: orange; }");
            mPublicHintLabel->setText("That master server rejected those credentials.");
            return;
        }
        RefreshMasterHostingState();
        mPublicInput->setChecked(true);
    });
}

void HostServerDialog::ApplyMasterServerSelection() {
    auto *registry = gApp->GetCore()->GetRegistry();
    QString masterUrl = mMasterServerBox->currentData().toString();

    if (masterUrl.isEmpty()) {
        registry->SetKeyValue<std::string>("emu.auth.type", "master");
        registry->SetKeyValue<bool>("emu.master.announce", false);
        registry->Save();
        return;
    }

    registry->SetKeyValue<std::string>("emu.auth.type", "slave");
    registry->SetKeyValue<std::string>("emu.auth.master", masterUrl.toStdString());
    // Slave mode is meaningless with the auth surface bypassed, so turn it on with the mode.
    registry->SetKeyValue<bool>("emu.auth.enabled", true);
    registry->SetKeyValue<bool>("emu.master.announce", mPublicInput->isChecked());
    registry->Save();
}
