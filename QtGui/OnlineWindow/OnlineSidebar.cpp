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
// File: OnlineSidebar.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of master servers and their pages
#include "OnlineSidebar.h"
#include "AddMasterServerDialog.h"
#include "MasterHttp.h"
#include "MasterServerStore.h"
#include "MasterLoginDialog.h"
#include "../Application.h"

#include <nlohmann/json.hpp>

#include <QAction>
#include <QMenu>
#include <QMessageBox>

using namespace NoobWarrior;

static constexpr int kRoleNodeKind = Qt::UserRole + 1;
static constexpr int kRoleMasterUrl = Qt::UserRole + 2;

static QStandardItem *MakeItem(const char *icon, const QString &text, OnlineNodeKind kind,
                               const QString &masterUrl) {
    auto *item = new QStandardItem(QIcon(icon), text);
    item->setEditable(false);
    item->setData(static_cast<int>(kind), kRoleNodeKind);
    item->setData(masterUrl, kRoleMasterUrl);
    return item;
}

OnlineSidebar::OnlineSidebar(QWidget *parent) : QDockWidget(parent) {
    setWindowTitle("Sidebar");
    InitWidgets();
    setFeatures(features() & ~DockWidgetClosable);
}

void OnlineSidebar::InitWidgets() {
    mView = new QTreeView(this);
    mView->setHeaderHidden(true);
    mView->setContextMenuPolicy(Qt::CustomContextMenu);

    mModel = new QStandardItemModel(this);
    mView->setModel(mModel);

    connect(mView, &QTreeView::customContextMenuRequested, this, &OnlineSidebar::OnContextMenu);
    connect(mView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) { OnSelectionChanged(current); });

    setWidget(mView);
    Reload();
}

QStandardItem *OnlineSidebar::AppendMaster(const QString &url, const QString &name, const QString &tooltip) {
    QStandardItem *masterRow = MakeItem(":/images/silk/server.png", name, OnlineNodeKind::MasterRoot, url);
    if (!tooltip.isEmpty())
        masterRow->setToolTip(tooltip);

    masterRow->appendRow(MakeItem(":/images/silk/controller.png", "Servers", OnlineNodeKind::Servers, url));
    masterRow->appendRow(MakeItem(":/images/silk/report_user.png", "Profile", OnlineNodeKind::Profile, url));
    masterRow->appendRow(MakeItem(":/images/silk/package.png", "Workshop", OnlineNodeKind::Workshop, url));

    mModel->invisibleRootItem()->appendRow(masterRow);
    return masterRow;
}

void OnlineSidebar::Reload() {
    // Remember where we were: Reload() runs on every add/remove, and losing the selection each time
    // would bounce the user back to the top of the tree.
    QString wantMaster = mSelectedMaster;
    OnlineNodeKind wantKind = mSelectedKind;

    mModel->clear();

    mModel->invisibleRootItem()->appendRow(
        MakeItem(":/images/silk/house.png", "Favorites & LAN Servers", OnlineNodeKind::Favorites, QString()));
    mModel->invisibleRootItem()->appendRow(
        MakeItem(":/images/silk/time.png", "Recently Played Servers", OnlineNodeKind::Recents, QString()));

    for (const MasterServerEntry &entry : MasterServerStore::Load()) {
        QString tooltip = entry.Url;
        if (!entry.Tagline.isEmpty())
            tooltip = entry.Tagline + "\n" + entry.Url;
        AppendMaster(entry.Url, entry.Name, tooltip);
    }

    mView->expandAll();

    // Re-select whatever was selected before, if it survived the rebuild.
    if (wantKind == OnlineNodeKind::None)
        return;

    auto matches = [&](QStandardItem *item) {
        return static_cast<OnlineNodeKind>(item->data(kRoleNodeKind).toInt()) == wantKind &&
               item->data(kRoleMasterUrl).toString() == wantMaster;
    };

    for (int i = 0; i < mModel->rowCount(); i++) {
        QStandardItem *row = mModel->item(i);
        if (matches(row)) {
            mView->setCurrentIndex(row->index());
            return;
        }
        for (int j = 0; j < row->rowCount(); j++) {
            if (matches(row->child(j))) {
                mView->setCurrentIndex(row->child(j)->index());
                return;
            }
        }
    }
}

void OnlineSidebar::OnSelectionChanged(const QModelIndex &current) {
    if (!current.isValid())
        return;

    auto kind = static_cast<OnlineNodeKind>(current.data(kRoleNodeKind).toInt());
    QString masterUrl = current.data(kRoleMasterUrl).toString();

    mSelectedKind = kind;
    mSelectedMaster = masterUrl;

    // Clicking the master itself is shorthand for its server list.
    if (kind == OnlineNodeKind::MasterRoot)
        kind = OnlineNodeKind::Servers;

    emit NodeSelected(kind, masterUrl);
}

void OnlineSidebar::OnContextMenu(const QPoint &pos) {
    QModelIndex index = mView->indexAt(pos);
    QMenu menu(this);

    QAction *addAction = menu.addAction(QIcon(":/images/silk/server_add.png"), "Add Master Server...");
    connect(addAction, &QAction::triggered, this, &OnlineSidebar::PromptAddMaster);

    QString masterUrl = index.isValid() ? index.data(kRoleMasterUrl).toString() : QString();
    if (!masterUrl.isEmpty()) {
        menu.addSeparator();

        if (MasterHttp::IsSignedIn(masterUrl)) {
            QAction *signOut = menu.addAction(QIcon(":/images/silk/door_out.png"), "Sign Out");
            connect(signOut, &QAction::triggered, this, [this, masterUrl]() {
                MasterHttp::SignOut(masterUrl);
                emit MastersChanged();
            });
        } else {
            QAction *signIn = menu.addAction(QIcon(":/images/silk/door_in.png"), "Sign In...");
            connect(signIn, &QAction::triggered, this, [this, masterUrl]() {
                MasterLoginDialog dialog(this, "Sign in", QString(), masterUrl, false);
                if (dialog.exec() != QDialog::Accepted)
                    return;
                MasterHttp::SignIn(this, dialog.MasterUrl(), dialog.Username(), dialog.Password(),
                                   [this](bool ok) {
                    if (!ok) {
                        QMessageBox::warning(this, "Sign in",
                                             "Could not sign in. Check your username and password.");
                        return;
                    }
                    emit MastersChanged();
                });
            });
        }

        QAction *refresh = menu.addAction(QIcon(":/images/silk/arrow_refresh.png"), "Refresh Details");
        connect(refresh, &QAction::triggered, this, [this, masterUrl]() { RefreshBranding(masterUrl); });

        QAction *remove = menu.addAction(QIcon(":/images/silk/server_delete.png"), "Remove Master Server");
        connect(remove, &QAction::triggered, this, [this, masterUrl]() { RemoveMaster(masterUrl); });
    }

    menu.exec(mView->viewport()->mapToGlobal(pos));
}

void OnlineSidebar::PromptAddMaster() {
    AddMasterServerDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (!MasterServerStore::Add(dialog.Result())) {
        QMessageBox::warning(this, "Add Master Server", "That master server is already in your list.");
        return;
    }

    // Land on the new master's server list rather than wherever we happened to be.
    mSelectedMaster = dialog.Result().Url;
    mSelectedKind = OnlineNodeKind::Servers;

    Reload();
    emit MastersChanged();
}

void OnlineSidebar::RemoveMaster(const QString &masterUrl) {
    std::optional<MasterServerEntry> entry = MasterServerStore::Find(masterUrl);
    QString name = entry.has_value() ? entry->Name : MasterServerStore::HostOf(masterUrl);

    QString question = QString("Remove \"%1\" from your list?\n\nYour saved sign-in for it is removed "
                               "too. Nothing on the master server itself is changed.").arg(name);
    if (QMessageBox::question(this, "Remove Master Server", question,
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;

    MasterHttp::SignOut(masterUrl);
    MasterServerStore::Remove(masterUrl);

    if (mSelectedMaster == masterUrl) {
        mSelectedMaster.clear();
        mSelectedKind = OnlineNodeKind::None;
    }

    Reload();
    emit MastersChanged();
}

void OnlineSidebar::RefreshBranding(const QString &masterUrl) {
    MasterHttp::Get(this, masterUrl, "/fed/v1/info", [this, masterUrl](const MasterResponse &response) {
        if (!response.Ok) {
            QMessageBox::warning(this, "Refresh Details",
                                 "Couldn't reach that master server: " +
                                     QString::fromStdString(response.Error));
            return;
        }

        nlohmann::json info = nlohmann::json::parse(response.Body, nullptr, false);
        if (info.is_discarded() || !info.is_object())
            return;

        MasterServerStore::UpdateBranding(masterUrl,
                                          QString::fromStdString(info.value("Name", std::string{})),
                                          QString::fromStdString(info.value("Domain", std::string{})),
                                          QString::fromStdString(info.value("Tagline", std::string{})));
        Reload();
    });
}
