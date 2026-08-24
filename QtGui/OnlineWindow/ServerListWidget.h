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
// File: ServerListWidget.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of servers retrieved from one master server
#pragma once
#include <QModelIndex>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>

#include <cstdint>
#include <vector>

namespace NoobWarrior {
struct GameServerInfo {
    QString EmulatorIp;
    int     EmulatorPort { 0 };   // the emulator's HTTPS port, which is what we connect to
    QString EmulatorName;
    // Who announced this emulator to the master, as name@domain. Empty when that master allows
    // anonymous hosting (master.auth.require_for_hosting off).
    QString HostIdentity;
    QString HostName;
    int64_t ServerId { 0 };
    int64_t PlaceId { 0 };
    QString PlaceName;
    int     GamePort { 0 };
    QString Version;
    int     Players { 0 };
    int     MaxPlayers { 0 };
    int64_t FirstSeen { 0 };
    int64_t LastSeen { 0 };
    
    int  PingMs { -1 };            // -1 when the emulator did not answer
    bool AuthEnabled { false };
    bool AllowGuests { false };
    bool Reachable { false };

    bool RequiresAccount() const { return AuthEnabled && !AllowGuests; }
};

class ServerListWidget : public QTreeView {
    Q_OBJECT
public:
    explicit ServerListWidget(QWidget *parent = nullptr);

    // Points the list at one master server. Empty clears it.
    void SetMaster(const QString &masterUrl);
    const QString &Master() const { return mMasterUrl; }

    void Refresh();

signals:
    void ServerSelected(const GameServerInfo &server);
    // The user asked to see the profile of whoever is hosting a listed server.
    void HostProfileRequested(const QString &identity);
    void SelectionCleared();
    // Human-readable progress ("Refreshing...", "3 servers", an error) for the window's status bar.
    void StatusChanged(const QString &status);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void OnDoubleClicked(const QModelIndex &index);
    void OnContextMenu(const QPoint &pos);

private:
    void InitWidgets();
    void Populate(const std::vector<GameServerInfo> &servers);
    void SetPlaceholder(const QString &text);

    QStandardItemModel *mModel { nullptr };
    QString mMasterUrl;
    QString mPlaceholder;
    std::vector<GameServerInfo> mServers;
    bool mRefreshing { false };
};
}
