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
// File: DatabaseDialog.h
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#pragma once
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>

#include "Sdk/EmuDbListWidget.h"

namespace NoobWarrior {
class DatabaseDialog : public QDialog {
    Q_OBJECT
public:
    DatabaseDialog(QWidget *parent = nullptr);
    void InitWidgets();
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    QGridLayout* mGridLayout;

    QFrame* mAvailableFrame;
    QVBoxLayout* mAvailableLayout;
    QLabel* mAvailableLabel;
    EmuDbListWidget* mAvailableList;

    QFrame* mSelectedFrame;
    QVBoxLayout* mSelectedLayout;
    QLabel* mSelectedLabel;
    EmuDbListWidget* mSelectedList;

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
    
    bool mCommitted = false;

    void SaveToRegistry();
    void RevertManager();
    void ImportFiles(const QStringList& filePaths, bool mountThem);
    bool IsPathMounted(const std::filesystem::path& filePath);
    void DeleteDatabases(const QList<QListWidgetItem*>& items);
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