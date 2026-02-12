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
// File: AccountPage.cpp
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include "AccountPage.h"
#include "../Application.h"
#include "../Dialog/AuthTokenDialog.h"
#include "NoobWarrior/Keychain/RbxKeychain.h"

using namespace NoobWarrior;

AccountPage::AccountPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void AccountPage::InitWidgets() {
    auto addTokenButton = new QPushButton("Add Account Using Token", this);
    connect(addTokenButton, &QPushButton::clicked, [this]() {
        AuthTokenDialog dialog(this);
        dialog.exec();
    });
    Layout->addWidget(addTokenButton);

    ListView = new QTreeView(this);
    ListView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    Layout->addWidget(ListView);

    ListModel = new QStandardItemModel(ListView);
    ListModel->setColumnCount(6);
    ListModel->setHorizontalHeaderLabels({"", "Active", "Id", "Name", "Expiration Date", "Expired?"});
    ListView->setModel(ListModel);
    ListView->setColumnHidden(0, true); // for some odd reason, the first column cannot be reordered. so we just make it blank and hide it. good job!

    ListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ListView, &QTreeView::customContextMenuRequested, [this](const QPoint &pos) {
        if (!ListView->indexAt(pos).isValid())
            return;

        QMenu menu(this);
        QAction *action1 = menu.addAction(QIcon(":/images/silk/user_go.png"), "Set as Active Account");
        QAction *action2 = menu.addAction(QIcon(":/images/silk/key.png"), "View Token");
        menu.addSeparator();
        QAction *action3 = menu.addAction(QIcon(":/images/silk/user_delete.png"), "Delete Account");

        connect(action1, &QAction::triggered, this, [this, pos]() {
            QItemSelectionModel *selectionModel = ListView->selectionModel();
            QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
            if (!selectedIndexes.empty()) {
                QStandardItem *primaryItem = ListModel->itemFromIndex(selectedIndexes.at(0));
                if (!primaryItem->data().isNull()) {
                    RbxKeychain *auth = gApp->GetCore()->GetRbxKeychain();
                    Account *acc = &auth->GetAccounts().at(primaryItem->data().toInt());
                    auth->SetActiveAccount(acc);
                    Refresh();
                }
            }
        });
        connect(action2, &QAction::triggered, this, []() {

        });

        menu.exec(ListView->mapToGlobal(pos));
    });

    Refresh();
}

void AccountPage::Refresh() {
    ListModel->removeRows(0, ListModel->rowCount());
    ListModel->setRowCount(0);

    RbxKeychain *auth = gApp->GetCore()->GetRbxKeychain();
    std::vector<Account> &accounts = auth->GetAccounts();
    for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
        Account &acc = accounts[i];
        QList<QStandardItem*> accRow;
        auto *emptyFieldThatContainsData = new QStandardItem();
        emptyFieldThatContainsData->setData(i);
        accRow
            << emptyFieldThatContainsData
            << new QStandardItem(QIcon(auth->GetActiveAccount() == &acc ? ":/images/silk/tick.png" : ""), "")
            << new QStandardItem(acc.Id > -1 ? QString::number(acc.Id) : "N/A")
            << new QStandardItem(!acc.Name.empty() ? QString::fromStdString(acc.Name) : "N/A")
            << new QStandardItem(acc.ExpireTimestamp > -1 ? "idk" : "N/A")
            << new QStandardItem(auth->HasAccountExpired(acc) ? "Yes" : "No");
        ListModel->appendRow(accRow);
    }
    for (auto &acc : accounts) {
        
    }
}

const QString AccountPage::GetTitle() {
    return "Roblox Accounts";
}

const QString AccountPage::GetDescription() {
    return "All of the Roblox accounts that noobWarrior is able to access. Make sure you aren't using a tampered version of noobWarrior by only downloading from official sources; if you did not do this then please proceed with caution.";
}

const QIcon AccountPage::GetIcon() {
    return QIcon(":/images/silk/folder_user.png");
}
