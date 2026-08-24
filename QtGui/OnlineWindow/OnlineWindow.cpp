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

#include <QAction>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>

using namespace NoobWarrior;

OnlineWindow::OnlineWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Online");
    InitWidgets();
}

void OnlineWindow::InitWidgets() {
    QMenu *viewMenu = menuBar()->addMenu("View");

    mToolBar = new QToolBar("Standard", this);
    mToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(mToolBar);

    QAction *directConnect = mToolBar->addAction(QIcon(":/images/silk/connect.png"), "Direct Connect");
    connect(directConnect, &QAction::triggered, this, []() {
        DirectConnectDialog dialog;
        dialog.exec();
    });

    QAction *addMaster = mToolBar->addAction(QIcon(":/images/silk/server_add.png"), "Add Master Server");
    mRefreshAction = mToolBar->addAction(QIcon(":/images/silk/arrow_refresh.png"), "Refresh");
    connect(mRefreshAction, &QAction::triggered, this, &OnlineWindow::RefreshCurrentPage);

    // Central area: one page per sidebar node kind, all pointed at the selected master.
    mPages = new QStackedWidget(this);

    mServerList = new ServerListWidget(mPages);
    mProfilePage = new ProfilePage(mPages);
    mWorkshopPage = new WorkshopPage(mPages);

    mPlaceholderPage = new QWidget(mPages);
    auto *placeholderLayout = new QVBoxLayout(mPlaceholderPage);
    mPlaceholderLabel = new QLabel(mPlaceholderPage);
    mPlaceholderLabel->setAlignment(Qt::AlignCenter);
    mPlaceholderLabel->setWordWrap(true);
    placeholderLayout->addWidget(mPlaceholderLabel);

    mPages->addWidget(mServerList);
    mPages->addWidget(mProfilePage);
    mPages->addWidget(mWorkshopPage);
    mPages->addWidget(mPlaceholderPage);
    setCentralWidget(mPages);

    mSidebar = new OnlineSidebar(this);
    mSidebar->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mSidebar);
    connect(addMaster, &QAction::triggered, mSidebar, &OnlineSidebar::PromptAddMaster);

    mServerInformationSidebar = new ServerInformationSidebar(this);
    mServerInformationSidebar->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea, mServerInformationSidebar);

    viewMenu->addAction(mSidebar->toggleViewAction());
    viewMenu->addAction(mServerInformationSidebar->toggleViewAction());

    connect(mSidebar, &OnlineSidebar::NodeSelected, this, &OnlineWindow::ShowNode);
    connect(mSidebar, &OnlineSidebar::MastersChanged, this, [this]() {
        // A sign-in or sign-out changes what every page is allowed to show.
        mProfilePage->Reload();
        mWorkshopPage->Reload();
    });

    connect(mServerList, &ServerListWidget::ServerSelected,
            mServerInformationSidebar, &ServerInformationSidebar::ShowServer);
    connect(mServerList, &ServerListWidget::SelectionCleared,
            mServerInformationSidebar, &ServerInformationSidebar::Clear);
    connect(mServerList, &ServerListWidget::StatusChanged, this,
            [this](const QString &status) { statusBar()->showMessage(status); });
    connect(mProfilePage, &ProfilePage::StatusChanged, this,
            [this](const QString &status) { statusBar()->showMessage(status); });
    connect(mWorkshopPage, &WorkshopPage::StatusChanged, this,
            [this](const QString &status) { statusBar()->showMessage(status); });
    connect(mProfilePage, &ProfilePage::SessionChanged, this, [this]() {
        // Signing in changes what the workshop offers (My Submissions, commenting, uploading).
        mWorkshopPage->Reload();
    });

    // Opening a host's profile borrows the Profile page in read-only mode rather than adding a
    // second, near-identical page. Back returns to the server list we came from.
    connect(mServerList, &ServerListWidget::HostProfileRequested, this, [this](const QString &identity) {
        mProfilePage->SetMaster(mCurrentMaster);
        mProfilePage->ViewIdentity(identity);
        mPages->setCurrentWidget(mProfilePage);
        mServerInformationSidebar->setVisible(false);
    });
    connect(mProfilePage, &ProfilePage::BackRequested, this, [this]() {
        ShowNode(OnlineNodeKind::Servers, mCurrentMaster);
    });

    statusBar();
    ShowNode(OnlineNodeKind::None, QString());
    resize(1100, 720);
}

void OnlineWindow::ShowNode(OnlineNodeKind kind, const QString &masterUrl) {
    mCurrentKind = kind;
    mCurrentMaster = masterUrl;

    // The server info dock only ever describes a row of the server list.
    mServerInformationSidebar->setVisible(kind == OnlineNodeKind::Servers);

    switch (kind) {
    case OnlineNodeKind::Servers:
        mServerList->SetMaster(masterUrl);
        mPages->setCurrentWidget(mServerList);
        break;
    case OnlineNodeKind::Profile:
        mProfilePage->SetMaster(masterUrl);
        mProfilePage->ViewSelf();
        mPages->setCurrentWidget(mProfilePage);
        break;
    case OnlineNodeKind::Workshop:
        mWorkshopPage->SetMaster(masterUrl);
        mPages->setCurrentWidget(mWorkshopPage);
        break;
    case OnlineNodeKind::Favorites:
        mPlaceholderLabel->setText("Favorites and LAN discovery are not implemented yet.\n\n"
                                   "Use Direct Connect to join a server emulator by address.");
        mPages->setCurrentWidget(mPlaceholderPage);
        break;
    case OnlineNodeKind::Recents:
        mPlaceholderLabel->setText("Recently played servers are not implemented yet.");
        mPages->setCurrentWidget(mPlaceholderPage);
        break;
    default:
        mPlaceholderLabel->setText("Add a master server from the toolbar or by right-clicking the\n"
                                   "sidebar, then pick Servers, Profile or Workshop under it.");
        mPages->setCurrentWidget(mPlaceholderPage);
        break;
    }

    mRefreshAction->setEnabled(kind == OnlineNodeKind::Servers || kind == OnlineNodeKind::Profile ||
                               kind == OnlineNodeKind::Workshop);
}

void OnlineWindow::RefreshCurrentPage() {
    switch (mCurrentKind) {
    case OnlineNodeKind::Servers:
        mServerList->Refresh();
        break;
    case OnlineNodeKind::Profile:
        mProfilePage->Reload();
        break;
    case OnlineNodeKind::Workshop:
        mWorkshopPage->Reload();
        break;
    default:
        break;
    }
}
