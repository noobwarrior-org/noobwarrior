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
// Description: Widget that contains a list of servers retrieved from the master server
#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QModelIndex>

namespace NoobWarrior {
class ServerListWidget : public QTreeView {
    Q_OBJECT
public:
    ServerListWidget(QWidget* parent = nullptr);

    // Kicks off an async refresh. Extracts URLs on the calling (main) thread,
    // then does the HTTP work on a background thread and posts results back.
    // Ignores calls that arrive while a refresh is already in flight.
    void RefreshFromMasters();
protected:
    void InitWidgets();
private slots:
    void OnDoubleClicked(const QModelIndex &index);
private:
    QStandardItemModel* mModel;
    bool mRefreshing {false};
};
}