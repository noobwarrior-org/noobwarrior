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

#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

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

    mAvailableList->setAcceptDrops(true);
    mAvailableList->setDropIndicatorShown(true);

    mAvailableList->setContextMenuPolicy(Qt::CustomContextMenu);
    mSelectedList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mAvailableList, &EmuDbListWidget::customContextMenuRequested,
            this, &DatabaseDialog::OnContextMenuRequested);
    connect(mSelectedList, &EmuDbListWidget::customContextMenuRequested,
            this, &DatabaseDialog::OnContextMenuRequested);

    connect(mSelectedList->model(), &QAbstractItemModel::rowsMoved,
            this, &DatabaseDialog::OnSelectedOrderChanged);
    connect(mAvailableList, &EmuDbListWidget::filesDropped,
            this, &DatabaseDialog::OnAvailableFilesDropped);
    connect(mSelectedList, &EmuDbListWidget::filesDropped,
            this, &DatabaseDialog::OnSelectedFilesDropped);

    mBottomLayout = new QHBoxLayout();
    mOpenFolderButton = new QPushButton("Open Databases Folder");
    mDiscardButton = new QPushButton("Discard");
    mSaveButton = new QPushButton("Save");

    mBottomLayout->addWidget(mOpenFolderButton);
    mBottomLayout->addStretch();
    mBottomLayout->addWidget(mDiscardButton);
    mBottomLayout->addWidget(mSaveButton);

    connect(mOpenFolderButton, &QPushButton::clicked, this, &DatabaseDialog::OnOpenFolder);
    connect(mDiscardButton, &QPushButton::clicked, this, &DatabaseDialog::OnDiscard);
    connect(mSaveButton, &QPushButton::clicked, this, &DatabaseDialog::OnSave);

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectorArrowFrame, 0, 1);
    mGridLayout->addWidget(mSelectedFrame, 0, 2);
    mGridLayout->addLayout(mBottomLayout, 1, 0, 1, 3);
}

void DatabaseDialog::OnMoveOneRight() {
    QList<QListWidgetItem*> items = mAvailableList->selectedItems();
    if (items.isEmpty())
        return;

    mDirty = true;
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    unsigned int priority = static_cast<unsigned int>(manager->GetMountedDatabases().size());
    for (auto* item : items) {
        std::filesystem::path filePath(item->toolTip().toStdString());
        manager->Mount(filePath, priority++);
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveAllRight() {
    if (mAvailableList->count() == 0)
        return;
    mDirty = true;
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    unsigned int priority = static_cast<unsigned int>(manager->GetMountedDatabases().size());
    for (int i = 0; i < mAvailableList->count(); i++) {
        std::filesystem::path filePath(mAvailableList->item(i)->toolTip().toStdString());
        manager->Mount(filePath, priority++);
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveOneLeft() {
    QList<EmuDb*> dbs = mSelectedList->GetSelectedDatabases();
    if (dbs.isEmpty())
        return;

    mDirty = true;
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    for (auto* db : dbs) {
        if (!db) continue;
        manager->Unmount(db);
        delete db;
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnMoveAllLeft() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::vector<EmuDb*> dbs = manager->GetMountedDatabases();
    if (dbs.empty())
        return;
    mDirty = true;
    for (auto* db : dbs) {
        manager->Unmount(db);
        delete db;
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnSelectedOrderChanged() {
    mDirty = true;
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::vector<EmuDb*> newOrder;
    for (int i = 0; i < mSelectedList->count(); i++) {
        EmuDb* db = reinterpret_cast<EmuDb*>(mSelectedList->item(i)->data(Qt::UserRole).value<quintptr>());
        if (db) newOrder.push_back(db);
    }
    manager->SetMountOrder(newOrder);
}

bool DatabaseDialog::IsPathMounted(const std::filesystem::path& filePath) {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    for (auto* db : manager->GetMountedDatabases()) {
        std::error_code ec;
        if (std::filesystem::equivalent(db->GetFilePath(), filePath, ec) && !ec)
            return true;
    }
    return false;
}

void DatabaseDialog::ImportFiles(const QStringList& filePaths, bool mountThem) {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    std::filesystem::path dbDir = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;

    for (const QString& src : filePaths) {
        std::filesystem::path srcPath(src.toStdString());
        std::filesystem::path destPath = dbDir / srcPath.filename();

        std::error_code ec;
        // Copy the file into the databases folder unless it is already there.
        if (!std::filesystem::equivalent(srcPath, destPath, ec)) {
            ec.clear();
            if (!std::filesystem::exists(destPath, ec))
                std::filesystem::copy_file(srcPath, destPath, ec);
            if (ec) {
                QMessageBox::warning(this, "Import failed",
                    QString("Could not import \"%1\":\n%2")
                        .arg(QString::fromStdString(srcPath.filename().string()))
                        .arg(QString::fromStdString(ec.message())));
                continue;
            }
        }

        if (mountThem && !IsPathMounted(destPath)) {
            unsigned int priority = static_cast<unsigned int>(manager->GetMountedDatabases().size());
            manager->Mount(destPath, priority);
            mDirty = true;
        }
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnAvailableFilesDropped(const QStringList& filePaths) {
    ImportFiles(filePaths, false);
}

void DatabaseDialog::OnSelectedFilesDropped(const QStringList& filePaths) {
    ImportFiles(filePaths, true);
}

void DatabaseDialog::OnContextMenuRequested(const QPoint& pos) {
    auto* list = qobject_cast<EmuDbListWidget*>(sender());
    if (list == nullptr)
        return;

    QListWidgetItem* clicked = list->itemAt(pos);
    if (clicked == nullptr)
        return;

    if (!clicked->isSelected()) {
        list->clearSelection();
        clicked->setSelected(true);
    }
    QList<QListWidgetItem*> items = list->selectedItems();

    QMenu menu;
    QAction* deleteAction = menu.addAction(QIcon(":/images/silk/cross.png"), "Delete");
    connect(deleteAction, &QAction::triggered, this, [this, items]() {
        DeleteDatabases(items);
    });
    menu.exec(list->viewport()->mapToGlobal(pos));
}

void DatabaseDialog::DeleteDatabases(const QList<QListWidgetItem*>& items) {
    if (items.isEmpty())
        return;

    QString message;
    if (items.size() == 1)
        message = QString("Are you sure you want to permanently delete \"%1\"?").arg(items.first()->text());
    else
        message = QString("Are you sure you want to permanently delete these %1 databases?").arg(items.size());
    message += "\n\nThis removes the database file from your computer and cannot be undone.";

    QMessageBox::StandardButton answer = QMessageBox::warning(
        this, "Permanently delete database", message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    mDirty = true;
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    for (auto* item : items) {
        std::filesystem::path path(item->toolTip().toStdString());
        if (path.empty())
            continue;

        EmuDb* db = reinterpret_cast<EmuDb*>(item->data(Qt::UserRole).value<quintptr>());
        if (db) {
            manager->Unmount(db);
            delete db;
        }

        std::error_code ec;
        std::filesystem::remove(path, ec);
        if (ec) {
            QMessageBox::warning(this, "Delete failed",
                QString("Could not delete \"%1\":\n%2")
                    .arg(QString::fromStdString(path.filename().string()))
                    .arg(QString::fromStdString(ec.message())));
        }
    }

    mAvailableList->Refresh();
    mSelectedList->Refresh();
}

void DatabaseDialog::OnOpenFolder() {
    std::filesystem::path dbDir = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(dbDir.string())));
}

void DatabaseDialog::OnSave() {
    SaveToRegistry();
    mCommitted = true;
    close();
}

void DatabaseDialog::OnDiscard() {
    RevertManager();
    mCommitted = true;
    close();
}

void DatabaseDialog::closeEvent(QCloseEvent* event) {
    if (!mCommitted && mDirty) {
        QMessageBox::StandardButton answer = QMessageBox::warning(
            this, "Unsaved Changes",
            "You have unsaved changes to your database selection.\n\nDo you want to save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        if (answer == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (answer == QMessageBox::Save)
            SaveToRegistry();
        else
            RevertManager();
    } else if (!mCommitted) {
        RevertManager();
    }
    QDialog::closeEvent(event);
}

void DatabaseDialog::RevertManager() {
    EmuDbManager* manager = gApp->GetCore()->GetEmuDbManager();
    manager->UnmountDatabases();
    manager->MountDatabases();
    manager->MountMasterDbIfNotAlreadyMounted();
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
