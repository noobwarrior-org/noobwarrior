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
// File: BackupDialog.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/Backup.h>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QSizePolicy>

namespace NoobWarrior {
class BackupDialog : public QDialog {
public:
    BackupDialog(QWidget *parent = nullptr);
    void InitWidgets();
    void UpdateWidgets();

    void InitOnlineUniverseWidgets();
    void InitOnlineAssetWidgets();
    void InitLocalFileWidgets();

    void StartBackup();
private:
    bool mChoseItemSource;

    Backup::ItemSource mSource;
    Backup::OnlineItemType mItemType;

    QVBoxLayout* mMainLayout;
    QVBoxLayout* mFrameLayout;
    QFrame* mFrame;
    QButtonGroup* mItemSourceButtonGroup;
    QHBoxLayout* mItemSourceRowLayout;

    QLabel* mItemTypeCaption;
    QComboBox* mItemTypeDropdown;

    QLabel* mIdCaption;
    QLineEdit* mIdField;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Online Universe Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////
    QFrame* mUniverseFrame;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Online Asset Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Local File Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////

    QDialogButtonBox* mButtons;
};
}