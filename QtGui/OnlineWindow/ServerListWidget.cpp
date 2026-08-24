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
// File: ServerListWidget.cpp
// Started by: Hattozo
// Started on: 11/6/2025
#include <cpr/cpr.h>

#include "ServerListWidget.h"
#include "MasterServerStore.h"
#include "../Application.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <map>
#include <thread>

#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTimer>

using namespace NoobWarrior;

// The row index into mServers, so a selection can be mapped back to the full record.
static constexpr int kRoleServerIndex = Qt::UserRole + 1;

ServerListWidget::ServerListWidget(QWidget *parent) : QTreeView(parent) {
    InitWidgets();
    connect(this, &QTreeView::doubleClicked, this, &ServerListWidget::OnDoubleClicked);
}

void ServerListWidget::InitWidgets() {
    mModel = new QStandardItemModel(this);
    mModel->setColumnCount(7);
    mModel->setHorizontalHeaderLabels({"Locked", "Ping", "Server", "Game", "Host", "Players", "Version"});
    setModel(mModel);
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);

    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(2, QHeaderView::Stretch);
    header()->setSectionResizeMode(3, QHeaderView::Stretch);
    header()->setSectionResizeMode(4, QHeaderView::Stretch);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &ServerListWidget::OnContextMenu);

    SetPlaceholder("Add a master server in the sidebar to see the game servers it lists.");

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &) {
                if (!current.isValid()) {
                    emit SelectionCleared();
                    return;
                }
                QStandardItem *first = mModel->item(current.row(), 0);
                if (first == nullptr)
                    return;
                int index = first->data(kRoleServerIndex).toInt();
                if (index < 0 || index >= static_cast<int>(mServers.size()))
                    return;
                emit ServerSelected(mServers[index]);
            });
}

void ServerListWidget::SetPlaceholder(const QString &text) {
    mPlaceholder = text;
    viewport()->update();
}

void ServerListWidget::paintEvent(QPaintEvent *event) {
    QTreeView::paintEvent(event);

    // An empty QTreeView is just a blank rectangle, which reads as "broken" rather than "nothing
    // here yet", so say which it is.
    if (mModel->rowCount() != 0 || mPlaceholder.isEmpty())
        return;

    QPainter painter(viewport());
    painter.setPen(palette().color(QPalette::Disabled, QPalette::Text));
    QRect area = viewport()->rect().adjusted(24, 24, -24, -24);
    painter.drawText(area, Qt::AlignCenter | Qt::TextWordWrap, mPlaceholder);
}

void ServerListWidget::SetMaster(const QString &masterUrl) {
    // Re-selecting the same master still refreshes: which game servers are up is the whole point of
    // the page, and it changes minute to minute.
    if (mMasterUrl != masterUrl) {
        mMasterUrl = masterUrl;
        mServers.clear();
        mModel->removeRows(0, mModel->rowCount());
        emit SelectionCleared();
    }

    if (mMasterUrl.isEmpty()) {
        SetPlaceholder("Add a master server in the sidebar to see the game servers it lists.");
        return;
    }
    Refresh();
}

// One probe of an emulator, shared by every game server running on it.
struct EmulatorProbe {
    int  PingMs { -1 };
    bool AuthEnabled { false };
    bool AllowGuests { false };
    bool Reachable { false };
};

static EmulatorProbe ProbeEmulator(const std::string &ip, int port) {
    EmulatorProbe probe {};

    // Same endpoint Application::ConnectToServer asks before prompting for a login, so the list
    // agrees with what actually happens on join. Timing it doubles as the ping.
    std::string url = "https://" + ip + ":" + std::to_string(port) + "/emu/v1/auth-info";
    auto started = std::chrono::steady_clock::now();
    cpr::Response res = cpr::Get(cpr::Url{url}, cpr::VerifySsl{false},
                                 cpr::Timeout{std::chrono::milliseconds(3000)});
    auto elapsed = std::chrono::steady_clock::now() - started;

    if (res.error.code != cpr::ErrorCode::OK)
        return probe;

    probe.Reachable = true;
    probe.PingMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

    if (res.status_code != 200)
        return probe;

    nlohmann::json info = nlohmann::json::parse(res.text, nullptr, false);
    if (info.is_discarded() || !info.is_object())
        return probe;

    probe.AuthEnabled = info.value("authEnabled", false);
    probe.AllowGuests = info.value("allowGuests", false);
    return probe;
}

void ServerListWidget::Refresh() {
    if (mRefreshing || mMasterUrl.isEmpty())
        return;

    mRefreshing = true;
    emit StatusChanged("Refreshing server list...");
    SetPlaceholder("Refreshing...");

    std::string url = MasterServerStore::NormalizeUrl(mMasterUrl).toStdString() + "/v1/servers";
    QPointer<ServerListWidget> self(this);

    std::thread([url = std::move(url), self]() {
        std::vector<GameServerInfo> servers;
        QString error;

        cpr::Response res = cpr::Get(cpr::Url{url}, cpr::VerifySsl{false},
                                     cpr::Timeout{std::chrono::milliseconds(5000)});
        if (res.error.code != cpr::ErrorCode::OK) {
            error = QString::fromStdString(res.error.message.empty()
                ? "Could not reach the master server" : res.error.message);
        } else if (res.status_code != 200) {
            error = QString("The master server answered HTTP %1").arg(res.status_code);
        } else {
            nlohmann::json list = nlohmann::json::parse(res.text, nullptr, false);
            if (list.is_discarded() || !list.is_array()) {
                error = "The master server sent a server list we could not read";
            } else {
                for (const auto &srv : list) {
                    GameServerInfo info {};
                    info.EmulatorIp = QString::fromStdString(srv.value("EmulatorIp", std::string{}));
                    info.EmulatorPort = srv.value("EmulatorPort", 53640);
                    info.EmulatorName = QString::fromStdString(srv.value("EmulatorName", std::string{}));
                    info.HostIdentity = QString::fromStdString(srv.value("HostIdentity", std::string{}));
                    info.HostName = QString::fromStdString(srv.value("HostName", std::string{}));
                    info.ServerId = srv.value("ServerId", static_cast<int64_t>(0));
                    info.PlaceId = srv.value("PlaceId", static_cast<int64_t>(0));
                    info.PlaceName = QString::fromStdString(srv.value("PlaceName", std::string{}));
                    info.GamePort = srv.value("GamePort", 0);
                    info.Version = QString::fromStdString(srv.value("Version", std::string{}));
                    info.Players = srv.value("Players", 0);
                    info.MaxPlayers = srv.value("MaxPlayers", 0);
                    info.FirstSeen = srv.value("FirstSeen", static_cast<int64_t>(0));
                    info.LastSeen = srv.value("LastSeen", static_cast<int64_t>(0));
                    if (info.EmulatorName.isEmpty())
                        info.EmulatorName = info.EmulatorIp;
                    servers.push_back(std::move(info));
                }

                // Several game servers usually share one emulator; probe each host once.
                std::map<std::pair<QString, int>, EmulatorProbe> probes;
                for (GameServerInfo &info : servers) {
                    auto key = std::make_pair(info.EmulatorIp, info.EmulatorPort);
                    auto it = probes.find(key);
                    if (it == probes.end())
                        it = probes.emplace(key, ProbeEmulator(info.EmulatorIp.toStdString(),
                                                               info.EmulatorPort)).first;
                    info.PingMs = it->second.PingMs;
                    info.AuthEnabled = it->second.AuthEnabled;
                    info.AllowGuests = it->second.AllowGuests;
                    info.Reachable = it->second.Reachable;
                }
            }
        }

        QTimer::singleShot(0, qApp, [self, servers = std::move(servers), error]() {
            if (!self)
                return;
            self->mRefreshing = false;

            if (!error.isEmpty()) {
                self->mServers.clear();
                self->mModel->removeRows(0, self->mModel->rowCount());
                self->SetPlaceholder(error);
                emit self->SelectionCleared();
                emit self->StatusChanged(error);
                return;
            }

            self->Populate(servers);
        });
    }).detach();
}

void ServerListWidget::Populate(const std::vector<GameServerInfo> &servers) {
    mServers = servers;
    mModel->removeRows(0, mModel->rowCount());
    emit SelectionCleared();

    for (size_t i = 0; i < mServers.size(); i++) {
        const GameServerInfo &info = mServers[i];

        auto *lockedItem = new QStandardItem();
        if (info.RequiresAccount()) {
            lockedItem->setIcon(QIcon(":/images/silk/lock.png"));
            lockedItem->setToolTip("You need an account on this host to join.");
        } else if (info.AuthEnabled) {
            lockedItem->setToolTip("Sign in, or join as a guest.");
        }
        lockedItem->setData(static_cast<int>(i), kRoleServerIndex);

        auto *pingItem = new QStandardItem(info.PingMs >= 0 ? QString::number(info.PingMs) : QString("-"));
        if (info.PingMs < 0)
            pingItem->setToolTip("The emulator did not answer a probe. It may be behind a firewall.");

        auto *serverItem = new QStandardItem(info.EmulatorName);
        serverItem->setToolTip(QString("%1:%2").arg(info.EmulatorIp).arg(info.EmulatorPort));

        QString gameName = info.PlaceName.isEmpty()
            ? QString("Place %1").arg(info.PlaceId)
            : info.PlaceName;
        auto *gameItem = new QStandardItem(gameName);

        // Masters that allow anonymous hosting report no owner at all.
        auto *hostItem = new QStandardItem(info.HostIdentity.isEmpty() ? QString("-")
                                                                       : info.HostIdentity);
        hostItem->setToolTip(info.HostIdentity.isEmpty()
            ? QString("This master server does not require an account to list a server.")
            : QString("Hosted by %1. Double-click to see their profile.").arg(info.HostIdentity));

        QString playersStr = info.MaxPlayers > 0
            ? QString("%1/%2").arg(info.Players).arg(info.MaxPlayers)
            : QString::number(info.Players);
        auto *playersItem = new QStandardItem(playersStr);

        auto *versionItem = new QStandardItem(info.Version);

        for (QStandardItem *item : {lockedItem, pingItem, serverItem, gameItem, hostItem, playersItem,
                                    versionItem})
            item->setEditable(false);

        mModel->appendRow({lockedItem, pingItem, serverItem, gameItem, hostItem, playersItem,
                           versionItem});
    }

    if (mServers.empty()) {
        SetPlaceholder("No game servers are online on this master server right now.");
        emit StatusChanged("No game servers online.");
        return;
    }

    SetPlaceholder(QString());
    emit StatusChanged(mServers.size() == 1
        ? QString("1 game server online.")
        : QString("%1 game servers online.").arg(mServers.size()));
}

static constexpr int kColumnHost = 4;

void ServerListWidget::OnDoubleClicked(const QModelIndex &index) {
    QStandardItem *first = mModel->item(index.row(), 0);
    if (first == nullptr)
        return;

    int serverIndex = first->data(kRoleServerIndex).toInt();
    if (serverIndex < 0 || serverIndex >= static_cast<int>(mServers.size()))
        return;

    const GameServerInfo &info = mServers[serverIndex];

    // The Host cell is about the person, not the server, so it opens their profile instead of
    // joining. Every other column keeps the obvious meaning of a double-click.
    if (index.column() == kColumnHost && !info.HostIdentity.isEmpty()) {
        emit HostProfileRequested(info.HostIdentity);
        return;
    }

    if (info.EmulatorIp.isEmpty())
        return;

    gApp->ConnectToServer(info.EmulatorIp.toStdString(), static_cast<uint16_t>(info.EmulatorPort));
}

void ServerListWidget::OnContextMenu(const QPoint &pos) {
    QModelIndex index = indexAt(pos);
    if (!index.isValid())
        return;

    QStandardItem *first = mModel->item(index.row(), 0);
    if (first == nullptr)
        return;
    int serverIndex = first->data(kRoleServerIndex).toInt();
    if (serverIndex < 0 || serverIndex >= static_cast<int>(mServers.size()))
        return;

    const GameServerInfo info = mServers[serverIndex];

    QMenu menu(this);
    QAction *join = menu.addAction(QIcon(":/images/silk/controller.png"), "Join Server");
    connect(join, &QAction::triggered, this, [info]() {
        if (info.EmulatorIp.isEmpty())
            return;
        gApp->ConnectToServer(info.EmulatorIp.toStdString(), static_cast<uint16_t>(info.EmulatorPort));
    });

    if (!info.HostIdentity.isEmpty()) {
        QAction *profile = menu.addAction(QIcon(":/images/silk/report_user.png"),
                                          QString("View Profile of %1").arg(info.HostIdentity));
        connect(profile, &QAction::triggered, this,
                [this, identity = info.HostIdentity]() { emit HostProfileRequested(identity); });
    }

    menu.exec(viewport()->mapToGlobal(pos));
}
