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
// File: PluginDialog.h
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#pragma once
#include <NoobWarrior/Plugin.h>

#include "../Sdk/PluginListWidget.h"

#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

#include <filesystem>

namespace NoobWarrior {
class PluginDialog : public QDialog {
    Q_OBJECT
public:
    PluginDialog(QWidget *parent = nullptr);
    void InitWidgets();
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QGridLayout* mGridLayout;

    QFrame* mAvailableFrame;
    QVBoxLayout* mAvailableLayout;
    QLabel* mAvailableLabel;
    PluginListWidget* mAvailableList;

    QFrame* mSelectedFrame;
    QVBoxLayout* mSelectedLayout;
    QLabel* mSelectedLabel;
    PluginListWidget* mSelectedList;
    QLabel* mSelectedHintLabel;

    QFrame* mSelectorArrowFrame;
    QVBoxLayout* mSelectorArrowLayout;
    QPushButton* mSelectorArrow_MoveOneRight;
    QPushButton* mSelectorArrow_MoveAllRight;
    QPushButton* mSelectorArrow_MoveOneLeft;
    QPushButton* mSelectorArrow_MoveAllLeft;

    QHBoxLayout* mBottomLayout;
    QPushButton* mOpenFolderButton;
    QPushButton* mDiscardButton;
    QPushButton* mSaveButton;

    // Working copy of plugins.selected, in mount order. Unlike DatabaseDialog there is nothing live
    // to mutate, a plugin can only be mounted on startup, so changes stay here until Save.
    QStringList mSelection;

    bool mCommitted = false;
    bool mDirty = false;
    bool mSeenDisclaimer = false;
    // Set while RefreshLists() is repopulating the lists so the row moves it causes don't get
    // mistaken for the user reordering the selection.
    bool mRefreshing = false;

    void RefreshLists();
    void UpdateButtonStates();
    void SaveToRegistry();
    void NotifyRestartRequired();
    void ImportFiles(const QStringList& filePaths, bool selectThem);
    void DeletePlugins(const QList<QListWidgetItem*>& items);
private slots:
    void OnMoveOneRight();
    void OnMoveAllRight();
    void OnMoveOneLeft();
    void OnMoveAllLeft();
    void OnSelectedOrderChanged();
    void OnAvailableFilesDropped(const QStringList& filePaths);
    void OnSelectedFilesDropped(const QStringList& filePaths);
    void OnOpenFolder();
    void OnSave();
    void OnDiscard();
    void OnContextMenuRequested(const QPoint& pos);
};
}
