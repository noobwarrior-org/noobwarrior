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
// File: OnlineWindow.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Window that contains features that the master server can present
#include "OnlineWindow.h"
#include "DirectConnectDialog.h"

#include <QMenuBar>
#include <QPushButton>

using namespace NoobWarrior;

OnlineWindow::OnlineWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Online");
    InitWidgets();
}

void OnlineWindow::InitWidgets() {
    menuBar()->addMenu("View");

    mToolBar = new QToolBar("Standard", this);
    addToolBar(mToolBar);

    auto *directConnect = new QPushButton("Direct Connect");
    connect(directConnect, &QPushButton::clicked, []() {
        DirectConnectDialog directConnect;
        directConnect.exec();
    });
    
    mToolBar->addWidget(directConnect);
    mToolBar->addWidget(new QPushButton("Refresh"));

    mServerList = new ServerListWidget(this);
    setCentralWidget(mServerList);

    mSidebar = new OnlineSidebar(this);
    mSidebar->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mSidebar);

    mServerInformationSidebar = new ServerInformationSidebar(this);
    mServerInformationSidebar->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea, mServerInformationSidebar);
}