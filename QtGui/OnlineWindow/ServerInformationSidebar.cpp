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
// File: ServerInformationSidebar.cpp
// Started by: Hattozo
// Started on: 4/24/2026
// Description: Details of the game server selected in the server list
#include "ServerInformationSidebar.h"
#include "../Application.h"

#include <QDateTime>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace NoobWarrior;

ServerInformationSidebar::ServerInformationSidebar(QWidget *parent) : QDockWidget(parent) {
    setWindowTitle("Server Information");
    InitWidgets();
}

ServerInformationSidebar::~ServerInformationSidebar() {}

QLabel *ServerInformationSidebar::AddRow(QFormLayout *form, const QString &label) {
    auto *value = new QLabel(mDetails);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(label, value);
    return value;
}

void ServerInformationSidebar::InitWidgets() {
    auto *container = new QWidget(this);
    auto *outer = new QVBoxLayout(container);

    mEmptyLabel = new QLabel("Select a game server to see its details.", container);
    mEmptyLabel->setWordWrap(true);
    mEmptyLabel->setAlignment(Qt::AlignTop);
    outer->addWidget(mEmptyLabel);

    mDetails = new QWidget(container);
    auto *detailsLayout = new QVBoxLayout(mDetails);
    detailsLayout->setContentsMargins(0, 0, 0, 0);

    mTitleLabel = new QLabel(mDetails);
    QFont titleFont = mTitleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    mTitleLabel->setFont(titleFont);
    mTitleLabel->setWordWrap(true);
    detailsLayout->addWidget(mTitleLabel);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    detailsLayout->addLayout(form);

    mHostLabel = AddRow(form, "Emulator");
    mAddressLabel = AddRow(form, "Address");
    mPlaceLabel = AddRow(form, "Place ID");
    mPlayersLabel = AddRow(form, "Players");
    mPingLabel = AddRow(form, "Ping");
    mVersionLabel = AddRow(form, "Version");
    mAccessLabel = AddRow(form, "Access");
    mUptimeLabel = AddRow(form, "Announced");

    mJoinButton = new QPushButton(QIcon(":/images/silk/controller.png"), "Join", mDetails);
    connect(mJoinButton, &QPushButton::clicked, this, [this]() {
        if (!mHasCurrent || mCurrent.EmulatorIp.isEmpty())
            return;
        gApp->ConnectToServer(mCurrent.EmulatorIp.toStdString(),
                              static_cast<uint16_t>(mCurrent.EmulatorPort));
    });
    detailsLayout->addWidget(mJoinButton);

    detailsLayout->addStretch();
    outer->addWidget(mDetails);
    outer->addStretch();

    setWidget(container);
    Clear();
}

void ServerInformationSidebar::Clear() {
    mHasCurrent = false;
    mCurrent = GameServerInfo {};
    mDetails->setVisible(false);
    mEmptyLabel->setVisible(true);
}

void ServerInformationSidebar::ShowServer(const GameServerInfo &server) {
    mCurrent = server;
    mHasCurrent = true;

    mTitleLabel->setText(server.PlaceName.isEmpty()
        ? QString("Place %1").arg(server.PlaceId)
        : server.PlaceName);

    mHostLabel->setText(server.EmulatorName);
    mAddressLabel->setText(QString("%1:%2").arg(server.EmulatorIp).arg(server.EmulatorPort));
    mPlaceLabel->setText(QString::number(server.PlaceId));

    mPlayersLabel->setText(server.MaxPlayers > 0
        ? QString("%1 / %2").arg(server.Players).arg(server.MaxPlayers)
        : QString::number(server.Players));

    if (!server.Reachable)
        mPingLabel->setText("No answer");
    else
        mPingLabel->setText(QString("%1 ms").arg(server.PingMs));

    mVersionLabel->setText(server.Version.isEmpty() ? "Unknown" : server.Version);

    if (!server.AuthEnabled)
        mAccessLabel->setText("Open (no sign-in)");
    else if (server.AllowGuests)
        mAccessLabel->setText("Sign in, or join as a guest");
    else
        mAccessLabel->setText("Account required");

    // FirstSeen is when the master first heard this game server announce itself, which is the
    // closest thing to an uptime the master knows.
    if (server.FirstSeen > 0) {
        QDateTime firstSeen = QDateTime::fromSecsSinceEpoch(server.FirstSeen);
        mUptimeLabel->setText(firstSeen.toString("MMM d, h:mm ap"));
    } else {
        mUptimeLabel->setText("Unknown");
    }

    mEmptyLabel->setVisible(false);
    mDetails->setVisible(true);
}
