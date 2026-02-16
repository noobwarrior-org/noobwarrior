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

#include "Application.h"
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <QDir>
#include <QFileInfo>

using namespace NoobWarrior;

EmuDbListWidget::EmuDbListWidget(Mode mode, QWidget* parent) : QListWidget(parent),
    mMode(mode)
{
    Refresh();
}

EmuDbListWidget::~EmuDbListWidget() {
    
}

void EmuDbListWidget::Refresh() {
    clear();

    if (mMode == Mode::ShowEntriesInDir) {
        std::filesystem::path dbPath = gApp->GetCore()->GetUserDataDir() / "databases";
        QDir directory(QString::fromStdString(dbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        directory.setNameFilters(filters);

        QFileInfoList fileList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : fileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), true);
            if (db.Fail())
                continue;

            QIcon icon;
            std::vector<unsigned char> iconData = db.GetIcon();
            if (!iconData.empty()) {
                QPixmap pixmap;
                pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
                icon = QIcon(pixmap);
            } else {
                icon = QIcon(":/images/silk/database.png");
            }

            QString title = QString::fromStdString(db.GetTitle());
            if (title.isEmpty())
                title = fileInfo.baseName();

            auto* item = new QListWidgetItem(icon, title, this);
            item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
            item->setToolTip(fileInfo.absoluteFilePath());
        }
    } else if (mMode == Mode::ShowMounted) {
        EmuDbManager *manager = gApp->GetCore()->GetEmuDbManager();
        std::vector<EmuDb*> dbs = manager->GetMountedDatabases();
        for (auto *db : dbs) {
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
            item->setData(Qt::UserRole, filePath);
            item->setToolTip(filePath);
        }
    }
}
