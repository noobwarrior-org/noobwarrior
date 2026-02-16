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

using namespace NoobWarrior;

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent),
    mCurrentDbPage(nullptr)
{
    setWindowTitle("Host Server");
    InitWidgets();
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);

    mDbListWidget = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);
    mDbPages = new QStackedWidget(this);

    mMainLayout->addWidget(mDbListWidget);
    mMainLayout->addWidget(mDbPages);

    connect(mDbListWidget, &QListWidget::itemSelectionChanged, [this]() {
        EmuDb* db = mDbListWidget->GetSelectedDatabase();
        if (db != nullptr) {
            if (mCurrentDbPage != nullptr) {
                mDbPages->removeWidget(mCurrentDbPage);
                mCurrentDbPage->deleteLater();
                mCurrentDbPage = nullptr;
            }
            mCurrentDbPage = new HostServerDbPage(db);
            mDbPages->addWidget(mCurrentDbPage);
            mDbPages->setCurrentWidget(mCurrentDbPage);
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
        gApp->LaunchClient({
            .NoobWarriorVersion = 1,
            .Type = ClientType::Server,
            .Hash = "07b64feec0bd47c1",
            .Version = "0.463.0.417004"
        });
    });
}

HostServerDbPage::HostServerDbPage(EmuDb* db, QWidget* parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    setLayout(layout);

    mUniverseListWidget = new ItemListWidget();
    layout->addWidget(mUniverseListWidget);

    mPlaceListWidget = new ItemListWidget();
    layout->addWidget(mPlaceListWidget);
}