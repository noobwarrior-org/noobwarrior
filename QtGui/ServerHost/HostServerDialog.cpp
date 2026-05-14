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

#include <QMessageBox>
#include <qsizepolicy.h>

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

    mButtonBox = new QDialogButtonBox();

    mStartServer = mButtonBox->addButton("Start Server", QDialogButtonBox::AcceptRole);
    mCloseButton = mButtonBox->addButton("Close", QDialogButtonBox::RejectRole);

    mLayout->addWidget(mPlaceInfoCardWidget);
    mLayout->addWidget(mServerSettingsWidget);
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
        close();
    });

    connect(mCloseButton, &QPushButton::clicked, [this]() {
        close();
    });
}
