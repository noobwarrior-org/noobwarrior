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
// File: DatabaseDialog.cpp
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#include "DatabaseDialog.h"
#include "Application.h"
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <filesystem>

using namespace NoobWarrior;

DatabaseDialog::DatabaseDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Databases");
    InitWidgets();
}

void DatabaseDialog::InitWidgets() {
    mGridLayout = new QGridLayout(this);

    mAvailableFrame = new QFrame();
    mAvailableLayout = new QVBoxLayout(mAvailableFrame);
    mAvailableLabel = new QLabel("Available");
    mAvailableList = new EmuDbListWidget(EmuDbListWidget::Mode::ShowNotMounted);

    mAvailableLayout->addWidget(mAvailableLabel);
    mAvailableLayout->addWidget(mAvailableList);

    mAvailableFrame->setAutoFillBackground(true);

    mSelectedFrame = new QFrame();
    mSelectedLayout = new QVBoxLayout(mSelectedFrame);
    mSelectedLabel = new QLabel("Selected");
    mSelectedList = new EmuDbListWidget(EmuDbListWidget::Mode::ShowMounted);

    mSelectedLayout->addWidget(mSelectedLabel);
    mSelectedLayout->addWidget(mSelectedList);

    mSelectedFrame->setAutoFillBackground(true);

    mSelectorArrowFrame = new QFrame();
    mSelectorArrowLayout = new QVBoxLayout(mSelectorArrowFrame);
    mSelectorArrow_MoveOneRight = new QPushButton(">");
    mSelectorArrow_MoveAllRight = new QPushButton(">>");
    mSelectorArrow_MoveOneLeft = new QPushButton("<");
    mSelectorArrow_MoveAllLeft = new QPushButton("<<");

    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveOneRight);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveAllRight);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveOneLeft);
    mSelectorArrowLayout->addWidget(mSelectorArrow_MoveAllLeft);

    connect(mSelectorArrow_MoveOneRight, &QPushButton::clicked, this, &DatabaseDialog::OnMoveOneRight);
    connect(mSelectorArrow_MoveAllRight, &QPushButton::clicked, this, &DatabaseDialog::OnMoveAllRight);
    connect(mSelectorArrow_MoveOneLeft,  &QPushButton::clicked, this, &DatabaseDialog::OnMoveOneLeft);
    connect(mSelectorArrow_MoveAllLeft,  &QPushButton::clicked, this, &DatabaseDialog::OnMoveAllLeft);

    mAvailableList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mSelectedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mSelectedList->setDragEnabled(true);
    mSelectedList->setAcceptDrops(true);
    mSelectedList->setDropIndicatorShown(true);
    mSelectedList->setDragDropMode(QAbstractItemView::InternalMove);

    connect(mSelectedList->model(), &QAbstractItemModel::rowsMoved,
            this, &DatabaseDialog::OnSelectedOrderChanged);

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectorArrowFrame, 0, 1);
    mGridLayout->addWidget(mSelectedFrame, 0, 2);
}

void DatabaseDialog::OnMoveOneRight() {
    QList<QListWidgetItem*> items = mAvailableList->selectedItems();
    if (items.isEmpty())
        return;

    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    unsigned int priority = static_cast<unsigned int>(manager->GetMountedDatabases().size());
    for (auto* item : items) {
        std::filesystem::path filePath(item->toolTip().toStdString());
        manager->Mount(filePath, priority++);
    }

    SaveToRegistry();
    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveAllRight() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    unsigned int priority = static_cast<unsigned int>(manager->GetMountedDatabases().size());
    for (int i = 0; i < mAvailableList->count(); i++) {
        std::filesystem::path filePath(mAvailableList->item(i)->toolTip().toStdString());
        manager->Mount(filePath, priority++);
    }

    SaveToRegistry();
    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveOneLeft() {
    QList<EmuDb*> dbs = mSelectedList->GetSelectedDatabases();
    if (dbs.isEmpty())
        return;

    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    for (auto* db : dbs) {
        if (!db) continue;
        manager->Unmount(db);
        delete db;
    }

    SaveToRegistry();
    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveAllLeft() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::vector<EmuDb*> dbs = manager->GetMountedDatabases();
    for (auto* db : dbs) {
        manager->Unmount(db);
        delete db;
    }

    SaveToRegistry();
    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnSelectedOrderChanged() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::vector<EmuDb*> newOrder;
    for (int i = 0; i < mSelectedList->count(); i++) {
        EmuDb* db = reinterpret_cast<EmuDb*>(mSelectedList->item(i)->data(Qt::UserRole).value<quintptr>());
        if (db) newOrder.push_back(db);
    }
    manager->SetMountOrder(newOrder);
    SaveToRegistry();
}

void DatabaseDialog::SaveToRegistry() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::vector<EmuDb*> mounted = manager->GetMountedDatabases();

    LuaState* lua = gApp->GetCore()->GetLuaState();
    sol::table mountedTbl = lua->create_table();
    int i = 1;
    for (auto* db : mounted)
        mountedTbl[i++] = db->GetFilePath().string();

    gApp->GetCore()->GetRegistry()->SetKeyValue("databases.mounted", mountedTbl);
}
