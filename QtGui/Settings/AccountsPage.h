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
// File: AccountsPage.h
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#pragma once
#include "SettingsPage.h"

#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTreeView>

namespace NoobWarrior {
class AccountsPage : public SettingsPage {
    Q_OBJECT
public:
    AccountsPage(QWidget *parent = nullptr);
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
    
    void RefreshRobloxAccounts();
protected:
    void InitWidgets();
    QWidget* BuildRobloxSection();
    QWidget* BuildMasterSection();
    QWidget* BuildServerAccountsSection();

    void RefreshMasterIdentity();
    void RefreshServerAccounts();
    void PromptCreateServerAccount();
private:
    QTreeView* mRbxView;
    QStandardItemModel* mRbxModel;

    QLabel* mMasterIdentityLabel;
    QPushButton* mMasterSignOutButton;

    QTreeView* mServerView;
    QStandardItemModel* mServerModel;
};
}
