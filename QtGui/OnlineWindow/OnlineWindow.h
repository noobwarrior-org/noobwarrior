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
#include "ProfilePage.h"
#include "ServerInformationSidebar.h"
#include "ServerListWidget.h"
#include "WorkshopPage.h"

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>

class QAction;
class QLabel;

namespace NoobWarrior {
class OnlineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit OnlineWindow(QWidget *parent = nullptr);

protected:
    void InitWidgets();

private:
    // Points every page at `masterUrl` and brings the one for `kind` to the front.
    void ShowNode(OnlineNodeKind kind, const QString &masterUrl);
    // Re-runs whatever the visible page does to load itself.
    void RefreshCurrentPage();

    QToolBar *mToolBar { nullptr };
    QAction *mRefreshAction { nullptr };
    OnlineSidebar *mSidebar { nullptr };
    ServerInformationSidebar *mServerInformationSidebar { nullptr };

    QStackedWidget *mPages { nullptr };
    ServerListWidget *mServerList { nullptr };
    ProfilePage *mProfilePage { nullptr };
    WorkshopPage *mWorkshopPage { nullptr };
    QWidget *mPlaceholderPage { nullptr };
    QLabel *mPlaceholderLabel { nullptr };

    OnlineNodeKind mCurrentKind { OnlineNodeKind::None };
    QString mCurrentMaster;
};
}
