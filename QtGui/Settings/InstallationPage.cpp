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
// File: InstallationPage.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#include "InstallationPage.h"
#include "../Application.h"

#include <NoobWarrior/Engine.h>

#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QDateTime>

#include <curl/curl.h>

using namespace NoobWarrior;

static std::map<EngineSide, QString> sIcons = {
    { EngineSide::Client, QString(":/images/client.png") },
    { EngineSide::Server, QString(":/images/server.png") },
    { EngineSide::Studio, QString(":/images/studio.png") }
};

InstallationPage::InstallationPage(QWidget *parent) : SettingsPage(parent),
    HorizontalLayout(nullptr),
    ListWidget(nullptr),
    StackedWidget(nullptr),
    CannotConnectLabel(nullptr)
{
    Init();
    InitWidgets();
}

void InstallationPage::InitWidgets() {
    HorizontalLayout = new QHBoxLayout();

    ListWidget = new QListWidget();
    ListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ListWidget->setFixedWidth(128);

    StackedWidget = new QStackedWidget();

    for (int i = 0; i < EngineSideCount; i++) {
        auto engineSide = static_cast<EngineSide>(i);
        auto engineSideItem = new QListWidgetItem(QIcon(sIcons[engineSide]), QString::fromStdString(EngineSideAsString(engineSide)), ListWidget);
        QFont font = engineSideItem->font();
        font.setPointSize(12);
        engineSideItem->setFont(font);

        auto engineView = new QTreeView(StackedWidget);
        engineView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        engineView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        auto engineModel = new QStandardItemModel(engineView);
        engineModel->setColumnCount(6);
        engineModel->setHorizontalHeaderLabels({"", "Latest", "Installed", "Version", "Hash", "Date"});
        engineView->setModel(engineModel);
        engineView->setColumnHidden(0, true); // for some odd reason, the first column cannot be reordered. so we just make it blank and hide it. good job!

        EngineVersionViewMap.emplace(engineSide, engineView);
        EngineVersionModelMap.emplace(engineSide, engineModel);
        StackedWidget->addWidget(engineView);

        connect(ListWidget, &QListWidget::currentItemChanged, [this, i, engineSideItem](QListWidgetItem *current, QListWidgetItem *previous) {
            if (current == engineSideItem)
                StackedWidget->setCurrentIndex(i);
        });
    }

    HorizontalLayout->addWidget(ListWidget);
    HorizontalLayout->addWidget(StackedWidget);

    IndexMessageLabel = new QLabel("");
    IndexMessageLabel->setWordWrap(true);
    IndexMessageLabel->setStyleSheet("QLabel { color: orange; }");
    IndexMessageLabel->setVisible(false);

    Layout->addWidget(IndexMessageLabel);
    Layout->addLayout(HorizontalLayout);

    CannotConnectLabel = new QLabel("");
    CannotConnectLabel->setWordWrap(true);
    CannotConnectLabel->setVisible(false);
    Layout->addWidget(CannotConnectLabel);
}

void InstallationPage::Refresh() {
    QString errMsg = "Could not connect to server to retrieve clients; no URL is set!";
    CannotConnectLabel->setText(errMsg);
    CannotConnectLabel->setVisible(true);
}

const QString InstallationPage::GetTitle() {
    return "Roblox Installations";
}

const QString InstallationPage::GetDescription() {
    return "All of the versions of Roblox that noobWarrior can install (or has installed) on your computer. Please note that you cannot just copy over an existing installation of Roblox, they have to be manually patched by us.";
}

const QIcon InstallationPage::GetIcon() {
    return QIcon(":/images/roblox_folder.png");
}
