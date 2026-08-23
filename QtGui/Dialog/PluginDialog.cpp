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
// File: PluginDialog.cpp
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#include "PluginDialog.h"

#include "../Application.h"
#include "NoobWarrior/Plugin.h"
#include <NoobWarrior/Macros.h>
#include <NoobWarrior/PluginManager.h>

#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileInfo>
#include <QIcon>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

using namespace NoobWarrior;

PluginDialog::PluginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Plugins");
    InitWidgets();
}

void PluginDialog::InitWidgets() {
    mGridLayout = new QGridLayout(this);

    mAvailableFrame = new QFrame();
    mAvailableLayout = new QVBoxLayout(mAvailableFrame);
    mAvailableLabel = new QLabel("Available");
    mAvailableList = new PluginListWidget(PluginListWidget::Mode::ShowNotSelected);

    mAvailableLayout->addWidget(mAvailableLabel);
    mAvailableLayout->addWidget(mAvailableList);

    mAvailableFrame->setAutoFillBackground(true);

    mSelectedFrame = new QFrame();
    mSelectedLayout = new QVBoxLayout(mSelectedFrame);
    mSelectedLabel = new QLabel("Selected");
    // The selected side also lists the built-in plugins, which are always mounted and show up as
    // locked entries the user cannot move out.
    mSelectedList = new PluginListWidget(PluginListWidget::Mode::ShowSelected,
                                         NW_NON_PRIVILEGED_PLUGINS | NW_PRIVILEGED_PLUGINS);

    mSelectedHintLabel = new QLabel("The higher the plugin, the earlier it loads. Built-in plugins are shown in italics and are always enabled.");
    mSelectedHintLabel->setEnabled(false);
    mSelectedHintLabel->setWordWrap(true);

    mSelectedLayout->addWidget(mSelectedLabel);
    mSelectedLayout->addWidget(mSelectedList);
    mSelectedLayout->addWidget(mSelectedHintLabel);

    mSelectedFrame->setAutoFillBackground(true);

    // Nothing is mounted or unmounted while this dialog is open, so the working copy starts out as
    // whatever the registry currently holds.
    mSelection = mSelectedList->GetSelection();

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

    connect(mSelectorArrow_MoveOneRight, &QPushButton::clicked, this, &PluginDialog::OnMoveOneRight);
    connect(mSelectorArrow_MoveAllRight, &QPushButton::clicked, this, &PluginDialog::OnMoveAllRight);
    connect(mSelectorArrow_MoveOneLeft,  &QPushButton::clicked, this, &PluginDialog::OnMoveOneLeft);
    connect(mSelectorArrow_MoveAllLeft,  &QPushButton::clicked, this, &PluginDialog::OnMoveAllLeft);

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
    connect(mAvailableList, &PluginListWidget::customContextMenuRequested,
            this, &PluginDialog::OnContextMenuRequested);
    connect(mSelectedList, &PluginListWidget::customContextMenuRequested,
            this, &PluginDialog::OnContextMenuRequested);

    connect(mAvailableList, &PluginListWidget::itemSelectionChanged,
            this, &PluginDialog::UpdateButtonStates);
    connect(mSelectedList, &PluginListWidget::itemSelectionChanged,
            this, &PluginDialog::UpdateButtonStates);

    connect(mSelectedList->model(), &QAbstractItemModel::rowsMoved,
            this, &PluginDialog::OnSelectedOrderChanged);
    connect(mAvailableList, &PluginListWidget::filesDropped,
            this, &PluginDialog::OnAvailableFilesDropped);
    connect(mSelectedList, &PluginListWidget::filesDropped,
            this, &PluginDialog::OnSelectedFilesDropped);

    mBottomLayout = new QHBoxLayout();
    mOpenFolderButton = new QPushButton("Open Plugins Folder");
    mDiscardButton = new QPushButton("Discard");
    mSaveButton = new QPushButton("Save");

    mBottomLayout->addWidget(mOpenFolderButton);
    mBottomLayout->addStretch();
    mBottomLayout->addWidget(mDiscardButton);
    mBottomLayout->addWidget(mSaveButton);

    connect(mOpenFolderButton, &QPushButton::clicked, this, &PluginDialog::OnOpenFolder);
    connect(mDiscardButton, &QPushButton::clicked, this, &PluginDialog::OnDiscard);
    connect(mSaveButton, &QPushButton::clicked, this, &PluginDialog::OnSave);

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectorArrowFrame, 0, 1);
    mGridLayout->addWidget(mSelectedFrame, 0, 2);
    mGridLayout->addLayout(mBottomLayout, 1, 0, 1, 3);

    RefreshLists();
}

void PluginDialog::RefreshLists() {
    mRefreshing = true;
    mAvailableList->SetSelection(mSelection);
    mSelectedList->SetSelection(mSelection);
    mAvailableList->Refresh();
    mSelectedList->Refresh();
    mRefreshing = false;

    UpdateButtonStates();
}

// The arrows say up front what can be moved, so a built-in plugin never has to be refused after the
// fact: highlighting one leaves "<" greyed out, because GetHighlightedFileNames() skips it.
void PluginDialog::UpdateButtonStates() {
    mSelectorArrow_MoveOneRight->setEnabled(!mAvailableList->GetHighlightedFileNames().isEmpty());
    mSelectorArrow_MoveAllRight->setEnabled(mAvailableList->count() > 0);
    mSelectorArrow_MoveOneLeft->setEnabled(!mSelectedList->GetHighlightedFileNames().isEmpty());
    mSelectorArrow_MoveAllLeft->setEnabled(!mSelection.isEmpty());
}

// Only worth saying once the user actually commits something: nothing takes effect while they are
// still shuffling the two lists around.
void PluginDialog::NotifyRestartRequired() {
    if (mSeenDisclaimer || !mDirty)
        return;
    mSeenDisclaimer = true;
    QMessageBox::warning(this, "Notice",
        "Changes to plugins do not apply until " NOOBWARRIOR_BRAND " is restarted.");
}

void PluginDialog::OnMoveOneRight() {
    QStringList fileNames = mAvailableList->GetHighlightedFileNames();
    if (fileNames.isEmpty())
        return;

    mDirty = true;
    // A newly selected plugin mounts last, so it wins against everything already in the list.
    for (const QString& fileName : fileNames) {
        if (!mSelection.contains(fileName))
            mSelection << fileName;
    }

    RefreshLists();
}

void PluginDialog::OnMoveAllRight() {
    QStringList fileNames = mAvailableList->GetFileNames();
    if (fileNames.isEmpty())
        return;

    mDirty = true;
    for (const QString& fileName : fileNames) {
        if (!mSelection.contains(fileName))
            mSelection << fileName;
    }

    RefreshLists();
}

void PluginDialog::OnMoveOneLeft() {
    QStringList fileNames = mSelectedList->GetHighlightedFileNames();
    if (fileNames.isEmpty())
        return;

    mDirty = true;
    for (const QString& fileName : fileNames)
        mSelection.removeAll(fileName);

    RefreshLists();
}

void PluginDialog::OnMoveAllLeft() {
    if (mSelection.isEmpty())
        return;

    mDirty = true;
    mSelection.clear();

    RefreshLists();
}

void PluginDialog::OnSelectedOrderChanged() {
    if (mRefreshing)
        return;
    mDirty = true;
    // GetFileNames() skips the locked built-in rows, so dragging around them can't smuggle one into
    // the selection.
    mSelection = mSelectedList->GetFileNames();

    RefreshLists();
}

void PluginDialog::ImportFiles(const QStringList& filePaths, bool selectThem) {
    std::filesystem::path pluginDir = gApp->GetCore()->GetUserDataDir() / NW_PATH_PLUGINS;

    std::error_code ec;
    std::filesystem::create_directories(pluginDir, ec);

    for (const QString& src : filePaths) {
        std::filesystem::path srcPath(src.toStdString());
        std::filesystem::path destPath = pluginDir / srcPath.filename();

        ec.clear();
        // Copy the plugin into the plugins folder unless it is already there.
        if (!std::filesystem::equivalent(srcPath, destPath, ec)) {
            ec.clear();
            if (!std::filesystem::exists(destPath, ec)) {
                ec.clear();
                if (std::filesystem::is_directory(srcPath, ec)) {
                    ec.clear();
                    std::filesystem::copy(srcPath, destPath,
                        std::filesystem::copy_options::recursive, ec);
                } else {
                    ec.clear();
                    std::filesystem::copy_file(srcPath, destPath, ec);
                }
            }
            if (ec) {
                QMessageBox::warning(this, "Import failed",
                    QString("Could not import \"%1\":\n%2")
                        .arg(QString::fromStdString(srcPath.filename().string()))
                        .arg(QString::fromStdString(ec.message())));
                continue;
            }
        }

        QString fileName = QString::fromStdString(destPath.filename().string());
        if (selectThem && !mSelection.contains(fileName)) {
            mSelection << fileName;
            mDirty = true;
        }
    }

    RefreshLists();
}

void PluginDialog::OnAvailableFilesDropped(const QStringList& filePaths) {
    ImportFiles(filePaths, false);
}

void PluginDialog::OnSelectedFilesDropped(const QStringList& filePaths) {
    ImportFiles(filePaths, true);
}

void PluginDialog::OnContextMenuRequested(const QPoint& pos) {
    auto* list = qobject_cast<PluginListWidget*>(sender());
    if (list == nullptr)
        return;

    QListWidgetItem* clicked = list->itemAt(pos);
    if (clicked == nullptr)
        return;

    // Built-in plugins live in the install directory and are not the user's to delete. Show the
    // menu anyway, greyed out, so right-clicking one answers the question instead of doing nothing.
    if (PluginListWidget::IsItemLocked(clicked)) {
        QMenu lockedMenu;
        QAction* lockedAction = lockedMenu.addAction(QIcon(":/images/silk/cross.png"), "Delete");
        lockedAction->setEnabled(false);
        lockedMenu.addSeparator();
        lockedMenu.addAction("Built-in plugin, always enabled")->setEnabled(false);
        lockedMenu.exec(list->viewport()->mapToGlobal(pos));
        return;
    }

    if (!clicked->isSelected()) {
        list->clearSelection();
        clicked->setSelected(true);
    }
    QList<QListWidgetItem*> items;
    for (QListWidgetItem* item : list->selectedItems()) {
        if (!PluginListWidget::IsItemLocked(item))
            items << item;
    }

    QMenu menu;
    QAction* deleteAction = menu.addAction(QIcon(":/images/silk/cross.png"), "Delete");
    connect(deleteAction, &QAction::triggered, this, [this, items]() {
        DeletePlugins(items);
    });
    menu.exec(list->viewport()->mapToGlobal(pos));
}

void PluginDialog::DeletePlugins(const QList<QListWidgetItem*>& items) {
    if (items.isEmpty())
        return;

    QString message;
    if (items.size() == 1)
        message = QString("Are you sure you want to permanently delete \"%1\"?").arg(items.first()->text());
    else
        message = QString("Are you sure you want to permanently delete these %1 plugins?").arg(items.size());
    message += "\n\nThis removes the plugin from your computer and cannot be undone.";

    QMessageBox::StandardButton answer = QMessageBox::warning(
        this, "Permanently delete plugin", message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    mDirty = true;
    for (auto* item : items) {
        std::filesystem::path path(PluginListWidget::GetItemFilePath(item).toStdString());
        if (path.empty())
            continue;

        mSelection.removeAll(PluginListWidget::GetItemFileName(item));

        std::error_code ec;
        // A plugin is either a .zip file or a whole directory, so remove_all() covers both.
        std::filesystem::remove_all(path, ec);
        if (ec) {
            QMessageBox::warning(this, "Delete failed",
                QString("Could not delete \"%1\":\n%2")
                    .arg(QString::fromStdString(path.filename().string()))
                    .arg(QString::fromStdString(ec.message())));
        }
    }

    RefreshLists();
}

void PluginDialog::OnOpenFolder() {
    std::filesystem::path pluginDir = gApp->GetCore()->GetUserDataDir() / NW_PATH_PLUGINS;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(pluginDir.string())));
}

void PluginDialog::OnSave() {
    SaveToRegistry();
    mCommitted = true;
    close();
}

void PluginDialog::OnDiscard() {
    // Nothing was mounted or unmounted, so dropping the working copy is all it takes.
    mCommitted = true;
    close();
}

void PluginDialog::closeEvent(QCloseEvent* event) {
    if (!mCommitted && mDirty) {
        QMessageBox::StandardButton answer = QMessageBox::warning(
            this, "Unsaved Changes",
            "You have unsaved changes to your plugin selection.\n\nDo you want to save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        if (answer == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (answer == QMessageBox::Save)
            SaveToRegistry();
    }
    QDialog::closeEvent(event);
}

void PluginDialog::SaveToRegistry() {
    NotifyRestartRequired();

    LuaState* lua = gApp->GetCore()->GetLuaState();
    sol::table selectedTbl = lua->create_table();
    int i = 1;
    for (const QString& fileName : mSelection)
        selectedTbl[i++] = fileName.toStdString();

    gApp->GetCore()->GetRegistry()->SetKeyValue("plugins.selected", selectedTbl);
}
