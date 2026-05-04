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

#include <QMessageBox>

using namespace NoobWarrior;

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Host Server");
    InitWidgets();
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);

    mDbListWidget = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);
    mTreeWidget = new QTreeWidget(this);

    mMainLayout->addWidget(mDbListWidget);
    mMainLayout->addWidget(mTreeWidget);

    connect(mDbListWidget, &QListWidget::itemSelectionChanged, [this]() {
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        if (db != nullptr) {
            Statement stmt = db->PrepareStatement("SELECT Name, StartPlaceId FROM Universe;");
            while (stmt.Step() == SQLITE_ROW) {
                auto *item = new QTreeWidgetItem(mTreeWidget);
                item->setText(0, QString::fromStdString(stmt.GetStringFromColumnIndex(0)));

                int64_t startPlaceId = stmt.GetInt64FromColumnIndex(1);

                Statement stmt2 = db->PrepareStatement("SELECT PlaceId FROM UniversePlace WHERE Id = ?;");
                while (stmt2.Step() == SQLITE_ROW) {

                }
            }
        }
    });
    mDbListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mDbListWidget->setCurrentRow(0);

    mButtonBox = new QDialogButtonBox();

    mStartServer = new QPushButton("Start Server");
    mButtonBox->addButton(mStartServer, QDialogButtonBox::AcceptRole);

    mCloseButton = new QPushButton("Close");
    mButtonBox->addButton(mCloseButton, QDialogButtonBox::RejectRole);

    mMainLayout->addWidget(mButtonBox);

    connect(mStartServer, &QPushButton::clicked, []() {
        gApp->LaunchEngine({
            .Engine = {
                .Type = EngineType::Roblox,
                .Side = EngineSide::Server,
                .Hash = "07b64feec0bd47c1",
                .Version = "0.463.0.417004"
            },
            .Port = 53640,
            .PlaceId = 1818
        });
    });
}
