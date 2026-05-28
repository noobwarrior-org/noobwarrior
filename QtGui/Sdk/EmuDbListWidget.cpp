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
// File: EmuDbListWidget.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "EmuDbListWidget.h"

#include "../Application.h"
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <QDir>
#include <QFileInfo>

using namespace NoobWarrior;

EmuDbListWidget::EmuDbListWidget(Mode mode, QWidget* parent) : QListWidget(parent),
    mMode(mode)
{
    setIconSize(QSize(48, 48));
    setStyleSheet(R"(
    QListWidget::item {
        padding: 4px 4px 4px 8px;
    }
    )");
    Refresh();
}

EmuDbListWidget::~EmuDbListWidget() {
    
}

void EmuDbListWidget::Refresh() {
    clear();

    if (mMode == Mode::ShowEntriesInDir) {
        std::filesystem::path dbPath = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;
        QDir directory(QString::fromStdString(dbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        directory.setNameFilters(filters);

        QFileInfoList fileList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : fileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), false);
            AddDatabase(&db, true);
        }
    } else if (mMode == Mode::ShowMounted) {
        EmuDbManager *manager = gApp->GetCore()->GetEmuDbManager();
        std::vector<EmuDb*> dbs = manager->GetMountedDatabases();
        for (auto *db : dbs) {
            AddDatabase(db);
        }
    } else if (mMode == Mode::ShowNotMounted) {
        EmuDbManager *manager = gApp->GetCore()->GetEmuDbManager();
        std::vector<EmuDb*> dbs = manager->GetMountedDatabases();

        std::filesystem::path dbPath = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;
        QDir directory(QString::fromStdString(dbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        directory.setNameFilters(filters);

        QFileInfoList fileList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : fileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), false);
            bool foundDbInMgr = false;
            for (auto *otherDb : dbs) {
                std::error_code ec;
                if (std::filesystem::equivalent(fileInfo.absoluteFilePath().toStdString(), otherDb->GetFilePath(), ec) && !ec) {
                    foundDbInMgr = true;
                    break;
                }
            }
            if (!foundDbInMgr) {
                AddDatabase(&db, true);
            }
        }
    }
}

// isTemp var exists to prevent dangling references from being set
void EmuDbListWidget::AddDatabase(EmuDb* db, bool isTemp) {
    if (db->Fail())
        return;
    QIcon icon;
    std::vector<unsigned char> iconData = db->GetIcon();
    if (!iconData.empty()) {
        QPixmap pixmap;
        pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
        icon = QIcon(pixmap);
    } else {
        icon = QIcon(":/images/silk/database.png");
    }

    QString fileName = QString::fromStdString(db->GetFileName());
    QString filePath = QString::fromStdString(db->GetFilePath().string());

    QString title = QString::fromStdString(db->GetTitle());
    if (title.isEmpty())
        title = fileName;

    auto* item = new QListWidgetItem(icon, title, this);
    if (!isTemp)
        item->setData(Qt::UserRole, QVariant::fromValue(db));
    item->setToolTip(filePath);
}

EmuDb* EmuDbListWidget::GetSelectedDatabase() {
    QListWidgetItem *item = currentItem();
    if (item != nullptr)
        return item->data(Qt::UserRole).value<EmuDb*>();
    return nullptr;
}

QList<EmuDb*> EmuDbListWidget::GetSelectedDatabases() {
    QList<EmuDb*> dbs;
    QList<QListWidgetItem*> items = selectedItems();
    for (auto *item : items) {
        dbs.push_back(item->data(Qt::UserRole).value<EmuDb*>());
    }
    return dbs;
}
