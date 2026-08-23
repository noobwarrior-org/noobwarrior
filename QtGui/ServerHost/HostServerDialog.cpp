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
#include "PlaceInfoCardWidget.h"
#include "Sdk/Item/UniverseTreeWidget.h"

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
    mMasterServerBox->addItem("None", -1);
    mServerSettingsFormLayout->addRow("Master Server", mMasterServerBox);

    mPublicInput = new QCheckBox();
    auto updatePublicInput = [this]() {
        if (mMasterServerBox->currentData(Qt::UserRole).value<int>() == -1) {
            mPublicInput->setChecked(false);
            mPublicInput->setDisabled(true);
        } else {
            mPublicInput->setDisabled(false);
        }
    };
    updatePublicInput();
    connect(mMasterServerBox, &QComboBox::currentIndexChanged, [updatePublicInput]() {
        updatePublicInput();
    });
    mServerSettingsFormLayout->addRow("Public", mPublicInput);

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
            const auto engine = mEngineCombo->currentData().value<Engine>();
            gApp->LaunchEngine({
                .Engine = engine,
                .Port = 53640,
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
