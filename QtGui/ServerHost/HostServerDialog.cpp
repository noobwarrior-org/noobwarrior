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
#include "Sdk/Item/UniverseDropdown.h"
#include "ServerSettingsWidget.h"

#include <QLabel>
#include <QMessageBox>
#include <qsizepolicy.h>

enum class HostEngineChoice {
    Rcc2021,
    Studio2023TeamTest,
};

using namespace NoobWarrior;

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Host Server");
    InitWidgets();
    resize(QSize(size().width() + 192, size().height() + 128));
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);
    mLayout = new QVBoxLayout();

    mDbListWidget = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);
    mUniverseDropdown = new UniverseDropdown(this);

    mMainLayout->addWidget(mDbListWidget);
    mMainLayout->addWidget(mUniverseDropdown);

    connect(mDbListWidget, &QListWidget::itemSelectionChanged, [this]() {
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        mUniverseDropdown->Populate(db);
    });
    mDbListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mDbListWidget->setCurrentRow(0);

    connect(mUniverseDropdown, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        mPlaceInfoCardWidget->Refresh(db, current->data(0, Qt::UserRole).toLongLong());
    });

    mPlaceInfoCardWidget = new PlaceInfoCardWidget();
    mServerSettingsWidget = new ServerSettingsWidget();

    auto* engineRow = new QHBoxLayout();
    engineRow->addWidget(new QLabel("Host engine:"));
    mEngineCombo = new QComboBox();
    // userData on each item carries the HostEngineChoice enum so the click
    // handler reads it directly without parsing the label text.
    mEngineCombo->addItem("RCCService 2021 (0.463)",            static_cast<int>(HostEngineChoice::Rcc2021));
    mEngineCombo->addItem("Studio 2023 Team Test (0.574)",      static_cast<int>(HostEngineChoice::Studio2023TeamTest));
    engineRow->addWidget(mEngineCombo, 1);

    mButtonBox = new QDialogButtonBox();

    mStartServer = mButtonBox->addButton("Start Server", QDialogButtonBox::AcceptRole);
    mCloseButton = mButtonBox->addButton("Close", QDialogButtonBox::RejectRole);

    mLayout->addWidget(mPlaceInfoCardWidget);
    mLayout->addWidget(mServerSettingsWidget);
    mLayout->addLayout(engineRow);
    mLayout->addWidget(mButtonBox);
    mMainLayout->addLayout(mLayout);

    connect(mStartServer, &QPushButton::clicked, [this]() {
        std::optional<int64_t> placeId = mUniverseDropdown->GetSelectedPlaceId();

        QList<QTreeWidgetItem*> items = mUniverseDropdown->selectedItems();
        if (items.size() <= 0) {
            QMessageBox::critical(nullptr, "Error", "You need to select a place!");
            return;
        }

        QTreeWidgetItem* item = items.at(0);

        int flagThatTellsUsIfSomethingWentWrong = item->data(0, Qt::UserRole + 1).toInt();
        if (flagThatTellsUsIfSomethingWentWrong != 0) {
            QMessageBox::critical(nullptr, "Error", flagThatTellsUsIfSomethingWentWrong == 1 ? "This universe has no places!" : "This universe does not have a set start place!");
            return;
        }

        if (!placeId.has_value()) {
            QMessageBox::critical(nullptr, "Error", "No selected place id!");
            return;
        }

        if (!mDbListWidget->GetSelectedDatabase()->DoesItemExist(ItemType::Asset, placeId.value())) {
            QMessageBox::critical(nullptr, "Error", "This place no longer exists in the database!");
            return;
        }

        const auto choice = static_cast<HostEngineChoice>(mEngineCombo->currentData().toInt());
        if (choice == HostEngineChoice::Rcc2021) {
            gApp->LaunchEngine({
                .Engine = {
                    .Type = EngineType::Roblox,
                    .Side = EngineSide::Server,
                    .Hash = "07b64feec0bd47c1",
                    .Version = "0.463.0.417004"
                },
                .Port = 53640,
                .PlaceId = placeId.value()
            });
        } else {
            gApp->LaunchEngine({
                .Engine = {
                    .Architecture = EngineArchitecture::x86_64,
                    .Type = EngineType::Roblox,
                    .Side = EngineSide::Studio,
                    .Hash = "c2e4d104afaf449c",
                    .Version = "0.574.0.5740446"
                },
                .Port = 53640,
                .PlaceId = placeId.value(),
                .LaunchSide = EngineSide::Server
            });
        }
        close();
    });

    connect(mCloseButton, &QPushButton::clicked, [this]() {
        close();
    });
}
