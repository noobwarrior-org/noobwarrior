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
// File: OnlineWindow.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Window that contains features that the master server can present
#pragma once
#include "OnlineSidebar.h"
#include "ServerInformationSidebar.h"
#include "ServerListWidget.h"

#include <QMainWindow>
#include <QToolBar>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QPushButton>

namespace NoobWarrior {
class OnlineWindow : public QMainWindow {
    Q_OBJECT
public:
    OnlineWindow(QWidget* parent = nullptr);
protected:
    void InitWidgets();
private:
    QToolBar* mToolBar;
    OnlineSidebar* mSidebar;
    ServerInformationSidebar* mServerInformationSidebar;
    ServerListWidget* mServerList;
    QTabWidget* ServersTab;
};
}