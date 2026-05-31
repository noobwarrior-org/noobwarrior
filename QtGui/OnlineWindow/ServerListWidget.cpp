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
#include "../Application.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <optional>

#include <QMessageBox>
#include <QPointer>
#include <QTimer>

using namespace NoobWarrior;

static constexpr int kRoleIp = Qt::UserRole + 1;
static constexpr int kRolePort = Qt::UserRole + 2;

ServerListWidget::ServerListWidget(QWidget* parent) : QTreeView(parent) {
    InitWidgets();
    connect(this, &QTreeView::doubleClicked, this, &ServerListWidget::OnDoubleClicked);
}

void ServerListWidget::InitWidgets() {
    mModel = new QStandardItemModel(this);
    mModel->setColumnCount(5);
    mModel->setHorizontalHeaderLabels({"Locked", "Ping", "Server", "Game", "Players"});
    setModel(mModel);
    setRootIsDecorated(false);
}

struct ServerRow {
    QString ip;
    int     port;
    QString name;
    QString game;
    int     players;
    int     maxPlayers;
};

void ServerListWidget::RefreshFromMasters() {
    if (mRefreshing)
        return;

    Registry* reg = gApp->GetCore()->GetRegistry();
    std::optional<sol::table> masters = reg->GetKeyValue<sol::table>("gui.master_servers");
    if (!masters.has_value())
        return;

    std::vector<std::string> urls;
    for (const auto &kv : masters.value()) {
        sol::table master = kv.second.as<sol::table>();
        sol::optional<std::string> urlOpt = master["url"];
        if (!urlOpt.has_value() || urlOpt->empty())
            continue;
        std::string url = *urlOpt;
        if (url.back() == '/')
            url.pop_back();
        urls.push_back(std::move(url));
    }

    if (urls.empty())
        return;

    mRefreshing = true;
    mModel->removeRows(0, mModel->rowCount());

    QPointer<ServerListWidget> self(this);

    std::thread([urls = std::move(urls), self]() {
        std::vector<ServerRow> rows;

        for (const auto &url : urls) {
            Out("ServerListWidget", "Requesting {}/v1/servers", url);
            cpr::Response res = cpr::Get(
                cpr::Url{url + "/v1/servers"},
                cpr::Timeout{std::chrono::milliseconds(5000)}
            );
            if (res.error.code != cpr::ErrorCode::OK || res.status_code != 200)
                continue;

            nlohmann::json list = nlohmann::json::parse(res.text, nullptr, false);
            if (list.is_discarded() || !list.is_array())
                continue;

            for (const auto &srv : list) {
                rows.push_back({
                    QString::fromStdString(srv.value("Ip", std::string{})),
                    srv.value("Port", 53640),
                    QString::fromStdString(srv.value("Name", std::string{})),
                    QString::fromStdString(srv.value("Game", std::string{})),
                    srv.value("Players", 0),
                    srv.value("MaxPlayers", 0),
                });
            }
        }
        
        QTimer::singleShot(0, qApp, [self, rows = std::move(rows)]() {
            if (!self)
                return;
            for (const auto &row : rows) {
                auto *lockedItem = new QStandardItem("");
                auto *pingItem   = new QStandardItem("");
                auto *serverItem = new QStandardItem(row.name.isEmpty() ? row.ip : row.name);
                auto *gameItem   = new QStandardItem(row.game);
                QString playersStr = row.maxPlayers > 0
                    ? QString("%1/%2").arg(row.players).arg(row.maxPlayers)
                    : QString::number(row.players);
                auto *playersItem = new QStandardItem(playersStr);

                lockedItem->setData(row.ip, kRoleIp);
                lockedItem->setData(row.port, kRolePort);

                self->mModel->appendRow({lockedItem, pingItem, serverItem, gameItem, playersItem});
            }
            self->mRefreshing = false;
        });
    }).detach();
}

void ServerListWidget::OnDoubleClicked(const QModelIndex &index) {
    QStandardItem *first = mModel->item(index.row(), 0);
    if (first == nullptr)
        return;
    QString ip = first->data(kRoleIp).toString();
    uint16_t port = static_cast<uint16_t>(first->data(kRolePort).toInt());
    if (ip.isEmpty())
        return;
    gApp->ConnectToServer(ip.toStdString(), port);
}