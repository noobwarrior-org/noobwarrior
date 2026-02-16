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
        auto engineSideItem = new QListWidgetItem(QIcon(sIcons[engineSide]), EngineSideAsTranslatableString(engineSide), ListWidget);
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
    nlohmann::json index;
    int res = gApp->GetCore()->RetrieveIndex(index);
    if (res != CURLE_OK) {
        auto url = gApp->GetCore()->GetConfig()->GetKeyValue<std::string>("internet.index");
        QString errMsg;
        switch (res) {
            default: errMsg = QString("Could not connect to server for file \"\"").arg(QString::fromStdString(url.value())); break;
            case -1: errMsg = "Could not connect to server to retrieve clients; no URL is set!"; break;
        }
        CannotConnectLabel->setText(errMsg);
        CannotConnectLabel->setVisible(true);
        return;
    }

#define REQUIRE(key) if (!index.contains(key)) { QMessageBox::critical(this, "Cannot Load Index", QString("This JSON is malformed since it does not contain a \"%1\" object.").arg(key)); return; }
    REQUIRE("Version")
    REQUIRE("Roblox")
#undef REQUIRE

    if (index.contains("Message") && !index["Message"].get<std::string>().empty()) {
        IndexMessageLabel->setText(QString::fromStdString(index["Message"].get<std::string>()));
        IndexMessageLabel->setVisible(true);
    }

    for (int i = 0; i <= EngineSideCount; i++) {
        auto engineSide = static_cast<EngineSide>(i);
        const char* clientTypeStr = EngineSideAsTranslatableString(engineSide);
        QStandardItemModel *engineVersionModel = EngineVersionModelMap.at(engineSide);
        engineVersionModel->removeRows(0, engineVersionModel->rowCount());
        engineVersionModel->setRowCount(0);

        if (!index["Roblox"].contains(clientTypeStr)) {
            QMessageBox::critical(this, "Error", QString("The index does not contain \"%1\" within the list.").arg(clientTypeStr));
            continue;
        }

        for (auto &engineInfo : index["Roblox"][clientTypeStr]) {
            std::string hash = engineInfo.at("Hash").get<std::string>();
            std::string version = engineInfo.at("Version").get<std::string>();
            std::string date = engineInfo.at("Date").get<std::string>();

            auto dateTime = QDateTime::fromString(QString::fromStdString(date), Qt::ISODate);

            Engine engine = {
                .Type = EngineType::Roblox,
                .Side = engineSide,
                .Hash = hash,
                .Version = version
            };

            QList<QStandardItem*> engineRow;
            engineRow
                << new QStandardItem("")
                << new QStandardItem(QIcon(":/images/silk/star.png"), "")
                << new QStandardItem(QIcon(gApp->GetCore()->IsEngineInstalled(engine) ? ":/images/silk/tick.png" : ":/images/silk/cross.png"), "")
                << new QStandardItem(QString::fromStdString(version))
                << new QStandardItem(QString::fromStdString(hash))
                << new QStandardItem(dateTime.toString("ddd MMMM d yyyy h:mm:ss AP"));
            engineVersionModel->appendRow(engineRow);
        }

        QTreeView *engineVersionView = EngineVersionViewMap.at(engineSide);
        for (int i = 0; i < engineVersionModel->columnCount(); i++) {
            engineVersionView->resizeColumnToContents(i);
        }
    }
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
