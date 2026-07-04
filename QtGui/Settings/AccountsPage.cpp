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
// File: AccountsPage.cpp
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#include "AccountsPage.h"
#include "Application.h"
#include "Dialog/AuthTokenDialog.h"
#include "OnlineWindow/MasterLoginDialog.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Keychain/RbxKeychain.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include <thread>

using namespace NoobWarrior;

AccountsPage::AccountsPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void AccountsPage::InitWidgets() {
    auto* content = new QWidget();
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->addWidget(BuildRobloxSection());
    contentLayout->addWidget(BuildMasterSection());
    contentLayout->addWidget(BuildEmuSection());
    contentLayout->addStretch();

    // Three sections plus two lists get tall, so keep the page scrollable below the title. Never scroll
    // horizontally. The content wraps to the viewport width (tree views keep their own inner scroll).
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    Layout->addWidget(scroll);
}

// A left-aligned, word-wrapping description label (so a long line never widens the page).
static QLabel* WrapLabel(const QString& text) {
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    return label;
}

QWidget* AccountsPage::BuildRobloxSection() {
    auto* box = new QGroupBox("Roblox accounts");
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(WrapLabel("You can log in to your Roblox account using noobWarrior. Please make sure you're using a copy of noobWarrior that has not been tampered with when doing that. This is necessary for fetching assets or backing up items from Roblox services."));

    auto* addButton = new QPushButton("Add account using token", box);
    connect(addButton, &QPushButton::clicked, this, [this]() {
        AuthTokenDialog dialog(this);
        dialog.exec();
    });
    layout->addWidget(addButton);

    mRbxView = new QTreeView(box);
    mRbxView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mRbxView->setMinimumHeight(140);
    mRbxView->setMaximumHeight(240);
    layout->addWidget(mRbxView);

    mRbxModel = new QStandardItemModel(mRbxView);
    mRbxModel->setColumnCount(5);
    mRbxModel->setHorizontalHeaderLabels({"", "Active", "Id", "Name", "Expired?"});
    mRbxView->setModel(mRbxModel);
    mRbxView->setColumnHidden(0, true); // hidden column stores the account index

    mRbxView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mRbxView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QModelIndex at = mRbxView->indexAt(pos);
        if (!at.isValid())
            return;
        QStandardItem* primary = mRbxModel->item(at.row(), 0);
        if (primary == nullptr || primary->data().isNull())
            return;
        int idx = primary->data().toInt();

        QMenu menu(this);
        QAction* setActive = menu.addAction(QIcon(":/images/silk/user_go.png"), "Set as active account");
        QAction* signOut = menu.addAction(QIcon(":/images/silk/cross.png"), "Use no account (sign out)");
        menu.addSeparator();
        QAction* del = menu.addAction(QIcon(":/images/silk/user_delete.png"), "Delete account");

        connect(setActive, &QAction::triggered, this, [this, idx]() {
            RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
            if (idx < static_cast<int>(auth->GetAccounts().size())) {
                auth->SetActiveAccount(&auth->GetAccounts().at(idx));
                auth->WriteToKeychain();
                RefreshRobloxAccounts();
            }
        });
        connect(signOut, &QAction::triggered, this, [this]() {
            RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
            auth->SetActiveAccount(nullptr); // no active Roblox account, but keep the list
            auth->WriteToKeychain();
            RefreshRobloxAccounts();
        });
        connect(del, &QAction::triggered, this, [this, idx]() {
            RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
            if (idx >= static_cast<int>(auth->GetAccounts().size()))
                return;
            QString name = QString::fromStdString(auth->GetAccounts().at(idx).Name);
            if (name.isEmpty()) name = "this account";
            if (QMessageBox::question(this, "Delete account", QString("Delete %1?").arg(name)) == QMessageBox::Yes) {
                auth->RemoveAccount(idx);
                auth->WriteToKeychain();
                RefreshRobloxAccounts();
            }
        });
        menu.exec(mRbxView->mapToGlobal(pos));
    });

    RefreshRobloxAccounts();
    return box;
}

QWidget* AccountsPage::BuildMasterSection() {
    auto* box = new QGroupBox("Master-server accounts");
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(WrapLabel("Accounts you've signed in to on master servers. The active one is the identity you join federated servers as."));

    auto* addButton = new QPushButton("Add account (sign in)...", box);
    connect(addButton, &QPushButton::clicked, this, [this]() {
        MasterLoginDialog dlg(this, "Sign in to a master server", QString(), QString(), false);
        if (dlg.exec() != QDialog::Accepted)
            return;
        Core* core = gApp->GetCore();
        QPointer<AccountsPage> self(this);
        std::string masterUrl = dlg.MasterUrl().toStdString();
        std::string username = dlg.Username().toStdString();
        std::string password = dlg.Password().toStdString();
        std::thread([self, core, masterUrl, username, password]() {
            bool ok = core->LoginToMaster(masterUrl, username, password);
            QTimer::singleShot(0, qApp, [self, ok]() {
                if (!self) return;
                if (!ok)
                    QMessageBox::critical(self, "Sign in failed", "That master server rejected those credentials.");
                self->RefreshMasterAccounts();
            });
        }).detach();
    });
    layout->addWidget(addButton);

    mMasterView = new QTreeView(box);
    mMasterView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMasterView->setMinimumHeight(120);
    mMasterView->setMaximumHeight(240);
    layout->addWidget(mMasterView);

    mMasterModel = new QStandardItemModel(mMasterView);
    mMasterModel->setColumnCount(4);
    mMasterModel->setHorizontalHeaderLabels({"", "Active", "Identity", "Master server"});
    mMasterView->setModel(mMasterModel);
    mMasterView->setColumnHidden(0, true); // hidden column stores the account index

    mMasterView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mMasterView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QModelIndex at = mMasterView->indexAt(pos);
        if (!at.isValid())
            return;
        QStandardItem* primary = mMasterModel->item(at.row(), 0);
        if (primary == nullptr || primary->data().isNull())
            return;
        int idx = primary->data().toInt();

        QMenu menu(this);
        QAction* setActive = menu.addAction(QIcon(":/images/silk/user_go.png"), "Set as active account");
        QAction* signOut = menu.addAction(QIcon(":/images/silk/cross.png"), "Use no account (sign out)");
        menu.addSeparator();
        QAction* remove = menu.addAction(QIcon(":/images/silk/user_delete.png"), "Remove account");

        connect(setActive, &QAction::triggered, this, [this, idx]() {
            MasterKeychain* kc = gApp->GetCore()->GetMasterKeychain();
            if (idx < static_cast<int>(kc->GetAccounts().size())) {
                kc->SetActiveAccount(&kc->GetAccounts().at(idx));
                kc->WriteToKeychain();
                RefreshMasterAccounts();
            }
        });
        connect(signOut, &QAction::triggered, this, [this]() {
            gApp->GetCore()->LogoutFromMaster(); // clears the active account, keeps the list
            RefreshMasterAccounts();
        });
        connect(remove, &QAction::triggered, this, [this, idx]() {
            MasterKeychain* kc = gApp->GetCore()->GetMasterKeychain();
            if (idx >= static_cast<int>(kc->GetAccounts().size()))
                return;
            QString name = QString::fromStdString(kc->GetAccounts().at(idx).Name);
            if (name.isEmpty()) name = "this account";
            if (QMessageBox::question(this, "Remove account", QString("Remove %1?").arg(name)) == QMessageBox::Yes) {
                kc->RemoveAccount(idx);
                kc->WriteToKeychain();
                RefreshMasterAccounts();
            }
        });
        menu.exec(mMasterView->mapToGlobal(pos));
    });

    RefreshMasterAccounts();
    return box;
}

QWidget* AccountsPage::BuildEmuSection() {
    auto* box = new QGroupBox("Saved server logins");
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(WrapLabel("Server emulators you've signed in to that use their own accounts. The login is saved so you can reconnect without signing in again."));

    mEmuView = new QTreeView(box);
    mEmuView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mEmuView->setMinimumHeight(120);
    mEmuView->setMaximumHeight(240);
    layout->addWidget(mEmuView);

    mEmuModel = new QStandardItemModel(mEmuView);
    mEmuModel->setColumnCount(3);
    mEmuModel->setHorizontalHeaderLabels({"", "Server", "Signed in as"});
    mEmuView->setModel(mEmuModel);
    mEmuView->setColumnHidden(0, true); // hidden column stores the account index

    mEmuView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mEmuView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        QModelIndex at = mEmuView->indexAt(pos);
        if (!at.isValid())
            return;
        QStandardItem* primary = mEmuModel->item(at.row(), 0);
        if (primary == nullptr || primary->data().isNull())
            return;
        int idx = primary->data().toInt();

        QMenu menu(this);
        QAction* remove = menu.addAction(QIcon(":/images/silk/user_delete.png"), "Forget this login");
        connect(remove, &QAction::triggered, this, [this, idx]() {
            EmuKeychain* kc = gApp->GetCore()->GetEmuKeychain();
            if (idx >= static_cast<int>(kc->GetAccounts().size()))
                return;
            kc->RemoveAccount(idx);
            kc->WriteToKeychain();
            RefreshEmuAccounts();
        });
        menu.exec(mEmuView->mapToGlobal(pos));
    });

    RefreshEmuAccounts();
    return box;
}

void AccountsPage::RefreshRobloxAccounts() {
    mRbxModel->removeRows(0, mRbxModel->rowCount());
    RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
    std::vector<Account>& accounts = auth->GetAccounts();
    for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
        Account& acc = accounts[i];
        auto* hidden = new QStandardItem();
        hidden->setData(i);
        QList<QStandardItem*> row;
        row << hidden
            << new QStandardItem(QIcon(auth->GetActiveAccount() == &acc ? ":/images/silk/tick.png" : ""), "")
            << new QStandardItem(acc.Id > -1 ? QString::number(acc.Id) : "N/A")
            << new QStandardItem(!acc.Name.empty() ? QString::fromStdString(acc.Name) : "N/A")
            << new QStandardItem(auth->HasAccountExpired(acc) ? "Yes" : "No");
        mRbxModel->appendRow(row);
    }
}

void AccountsPage::RefreshMasterAccounts() {
    mMasterModel->removeRows(0, mMasterModel->rowCount());
    MasterKeychain* kc = gApp->GetCore()->GetMasterKeychain();
    std::vector<Account>& accounts = kc->GetAccounts();
    for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
        Account& acc = accounts[i];
        auto* hidden = new QStandardItem();
        hidden->setData(i);
        QList<QStandardItem*> row;
        row << hidden
            << new QStandardItem(QIcon(kc->GetActiveAccount() == &acc ? ":/images/silk/tick.png" : ""), "")
            << new QStandardItem(!acc.Name.empty() ? QString::fromStdString(acc.Name) : "N/A")
            << new QStandardItem(QString::fromStdString(acc.Url));
        mMasterModel->appendRow(row);
    }
}

void AccountsPage::RefreshEmuAccounts() {
    mEmuModel->removeRows(0, mEmuModel->rowCount());
    EmuKeychain* kc = gApp->GetCore()->GetEmuKeychain();
    std::vector<Account>& accounts = kc->GetAccounts();
    for (int i = 0; i < static_cast<int>(accounts.size()); ++i) {
        Account& acc = accounts[i];
        auto* hidden = new QStandardItem();
        hidden->setData(i);
        QList<QStandardItem*> row;
        row << hidden
            << new QStandardItem(QString::fromStdString(acc.Name)) // host (ip:port)
            << new QStandardItem(QString::fromStdString(acc.DisplayName)); // username
        mEmuModel->appendRow(row);
    }
}

const QString AccountsPage::GetTitle() {
    return "Accounts";
}

const QString AccountsPage::GetDescription() {
    return "Your Roblox accounts, your master-server accounts, and saved logins for servers you connect to.";
}

const QIcon AccountsPage::GetIcon() {
    return QIcon(":/images/silk/folder_user.png");
}
