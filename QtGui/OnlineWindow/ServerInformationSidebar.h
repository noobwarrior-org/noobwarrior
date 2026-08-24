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
// File: ServerInformationSidebar.h
// Started by: Hattozo
// Started on: 4/24/2026
// Description: Details of the game server selected in the server list
#pragma once
#include "ServerListWidget.h"

#include <QDockWidget>

class QFormLayout;
class QLabel;
class QPushButton;
class QWidget;

namespace NoobWarrior {
class ServerInformationSidebar : public QDockWidget {
    Q_OBJECT
public:
    explicit ServerInformationSidebar(QWidget *parent = nullptr);
    ~ServerInformationSidebar();

public slots:
    void ShowServer(const GameServerInfo &server);
    void Clear();

protected:
    void InitWidgets();

private:
    QLabel *AddRow(QFormLayout *form, const QString &label);

    QWidget *mDetails { nullptr };
    QLabel *mEmptyLabel { nullptr };
    QLabel *mTitleLabel { nullptr };
    QLabel *mHostLabel { nullptr };
    QLabel *mAddressLabel { nullptr };
    QLabel *mPlaceLabel { nullptr };
    QLabel *mPlayersLabel { nullptr };
    QLabel *mPingLabel { nullptr };
    QLabel *mVersionLabel { nullptr };
    QLabel *mAccessLabel { nullptr };
    QLabel *mUptimeLabel { nullptr };
    QPushButton *mJoinButton { nullptr };

    GameServerInfo mCurrent {};
    bool mHasCurrent { false };
};
}
