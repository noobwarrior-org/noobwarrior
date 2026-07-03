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
    contentLayout->addWidget(BuildServerAccountsSection());
    contentLayout->addStretch();

    // Three sections plus two lists get tall, so keep the page scrollable below the title. Never scroll
    // horizontally — the content wraps to the viewport width (tree views keep their own inner scroll).
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
    layout->addWidget(WrapLabel("Real Roblox.com accounts (via .ROBLOSECURITY) that noobWarrior can act as when talking to Roblox services."));

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
        menu.addSeparator();
        QAction* del = menu.addAction(QIcon(":/images/silk/user_delete.png"), "Delete account");

        connect(setActive, &QAction::triggered, this, [this, idx]() {
            RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
            if (idx < static_cast<int>(auth->GetAccounts().size())) {
                auth->SetActiveAccount(&auth->GetAccounts().at(idx));
                RefreshRobloxAccounts();
            }
        });
        connect(del, &QAction::triggered, this, [this, idx]() {
            RbxKeychain* auth = gApp->GetCore()->GetRbxKeychain();
            if (idx >= static_cast<int>(auth->GetAccounts().size()))
                return;
            QString name = QString::fromStdString(auth->GetAccounts().at(idx).Name);
            if (name.isEmpty()) name = "this account";
            if (QMessageBox::question(this, "Delete account", QString("Delete %1?").arg(name)) == QMessageBox::Yes) {
                auth->RemoveAccount(idx);
                RefreshRobloxAccounts();
            }
        });
        menu.exec(mRbxView->mapToGlobal(pos));
    });

    RefreshRobloxAccounts();
    return box;
}

QWidget* AccountsPage::BuildMasterSection() {
    auto* box = new QGroupBox("Your master-server account");
    auto* layout = new QVBoxLayout(box);

    mMasterIdentityLabel = new QLabel(box);
    mMasterIdentityLabel->setWordWrap(true);
    layout->addWidget(mMasterIdentityLabel);

    auto* buttons = new QHBoxLayout();
    auto* signIn = new QPushButton("Sign in...", box);
    mMasterSignOutButton = new QPushButton("Sign out", box);
    buttons->addWidget(signIn);
    buttons->addWidget(mMasterSignOutButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(signIn, &QPushButton::clicked, this, [this]() {
        Registry* reg = gApp->GetCore()->GetRegistry();
        QString current = QString::fromStdString(reg->GetKeyValue<std::string>("online.master_url").value_or(""));
        MasterLoginDialog dlg(this, "Sign in to your master server", QString(), current, false);
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
                    QMessageBox::critical(self, "Sign in failed", "Your master server rejected those credentials.");
                self->RefreshMasterIdentity();
            });
        }).detach();
    });
    connect(mMasterSignOutButton, &QPushButton::clicked, this, [this]() {
        gApp->GetCore()->LogoutFromMaster();
        RefreshMasterIdentity();
    });

    RefreshMasterIdentity();
    return box;
}

QWidget* AccountsPage::BuildServerAccountsSection() {
    auto* box = new QGroupBox("Server accounts (hosted here)");
    auto* layout = new QVBoxLayout(box);
    layout->addWidget(WrapLabel("Accounts players log in with when they join this server (master mode). Stored in this server's master database."));

    auto* buttons = new QHBoxLayout();
    auto* createButton = new QPushButton("Create account...", box);
    auto* deleteButton = new QPushButton("Delete account", box);
    buttons->addWidget(createButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    mServerView = new QTreeView(box);
    mServerView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mServerView->setMinimumHeight(140);
    mServerView->setMaximumHeight(240);
    layout->addWidget(mServerView);

    mServerModel = new QStandardItemModel(mServerView);
    mServerModel->setColumnCount(3);
    mServerModel->setHorizontalHeaderLabels({"Id", "Name", "Display Name"});
    mServerView->setModel(mServerModel);

    connect(createButton, &QPushButton::clicked, this, [this]() { PromptCreateServerAccount(); });
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        QModelIndexList sel = mServerView->selectionModel() ? mServerView->selectionModel()->selectedRows() : QModelIndexList();
        if (sel.isEmpty()) {
            QMessageBox::information(this, "Delete account", "Select an account to delete.");
            return;
        }
        QStandardItem* idItem = mServerModel->item(sel.first().row(), 0);
        if (idItem == nullptr)
            return;
        int64_t id = idItem->data().toLongLong();
        QString name = mServerModel->item(sel.first().row(), 1)->text();
        if (QMessageBox::question(this, "Delete account", QString("Delete account \"%1\"?").arg(name)) != QMessageBox::Yes)
            return;
        EmuDb* master = gApp->GetCore()->GetEmuDbManager()->GetMasterDatabase();
        AuthUtil::DeleteLocalAccount(master, id);
        RefreshServerAccounts();
    });

    RefreshServerAccounts();
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

void AccountsPage::RefreshMasterIdentity() {
    Registry* reg = gApp->GetCore()->GetRegistry();
    QString identity = QString::fromStdString(reg->GetKeyValue<std::string>("online.identity").value_or(""));
    bool signedIn = !identity.isEmpty();
    mMasterIdentityLabel->setText(signedIn
        ? QString("Signed in as <b>%1</b>. This identity is reused to join federated servers.").arg(identity.toHtmlEscaped())
        : QString("Not signed in. Sign in to any master server to join federated servers with one login."));
    mMasterSignOutButton->setEnabled(signedIn);
}

void AccountsPage::RefreshServerAccounts() {
    mServerModel->removeRows(0, mServerModel->rowCount());
    EmuDb* master = gApp->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    for (const AuthUtil::LocalAccount& acc : AuthUtil::ListLocalAccounts(master)) {
        auto* idItem = new QStandardItem(QString::number(acc.id));
        idItem->setData(static_cast<qlonglong>(acc.id));
        QList<QStandardItem*> row;
        row << idItem
            << new QStandardItem(QString::fromStdString(acc.name))
            << new QStandardItem(QString::fromStdString(acc.displayName));
        mServerModel->appendRow(row);
    }
}

void AccountsPage::PromptCreateServerAccount() {
    EmuDb* master = gApp->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (master == nullptr) {
        QMessageBox::critical(this, "Create account", "No master database is mounted.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Create server account");
    auto* form = new QFormLayout(&dlg);
    auto* nameInput = new QLineEdit(&dlg);
    auto* passInput = new QLineEdit(&dlg);
    passInput->setEchoMode(QLineEdit::Password);
    auto* displayInput = new QLineEdit(&dlg);
    displayInput->setPlaceholderText("(optional, defaults to username)");
    form->addRow("Username", nameInput);
    form->addRow("Password", passInput);
    form->addRow("Display name", displayInput);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return;

    std::string name = nameInput->text().trimmed().toStdString();
    std::string password = passInput->text().toStdString();
    std::string display = displayInput->text().trimmed().toStdString();
    if (name.empty() || password.empty()) {
        QMessageBox::warning(this, "Create account", "Enter a username and password.");
        return;
    }
    if (AuthUtil::LocalAccountExists(master, name)) {
        QMessageBox::warning(this, "Create account", "That username is already taken.");
        return;
    }
    if (!AuthUtil::CreateLocalAccount(master, name, password, display)) {
        QMessageBox::critical(this, "Create account", "Failed to create the account.");
        return;
    }
    RefreshServerAccounts();
}

const QString AccountsPage::GetTitle() {
    return "Accounts";
}

const QString AccountsPage::GetDescription() {
    return "Your Roblox accounts, your master-server sign-in, and the accounts hosted on this server.";
}

const QIcon AccountsPage::GetIcon() {
    return QIcon(":/images/silk/folder_user.png");
}
