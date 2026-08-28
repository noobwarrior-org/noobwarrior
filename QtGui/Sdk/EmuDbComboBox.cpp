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
// File: EmuDbComboBox.cpp
// Started by: Hattozo
// Started on: 5/27/2026
// Description:
#include "EmuDbComboBox.h"
#include "Application.h"

#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <QDir>
#include <QFileInfoList>

using namespace NoobWarrior;

EmuDbComboBox::EmuDbComboBox(Mode mode, QWidget* parent) : QComboBox(parent),
    mMode(mode)
{
    Refresh();
}

EmuDbComboBox::~EmuDbComboBox() {

}

void EmuDbComboBox::Refresh() {
    clear();

    if (mMode == Mode::ShowEntriesInDir) {
        std::filesystem::path installDbPath = gApp->GetCore()->GetInstallDataDir() / NW_PATH_DATABASES;
        std::filesystem::path userDbPath = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;

        QDir installDir(QString::fromStdString(installDbPath.string()));
        QDir userDir(QString::fromStdString(userDbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        installDir.setNameFilters(filters);
        userDir.setNameFilters(filters);

        QFileInfoList installFileList = userDir.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : installFileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), false);
            AddDatabase(&db, true);
        }

        QFileInfoList userFileList = userDir.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : userFileList) {
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

        std::filesystem::path installDbPath = gApp->GetCore()->GetInstallDataDir() / NW_PATH_DATABASES;
        std::filesystem::path userDbPath = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;

        QDir installDir(QString::fromStdString(installDbPath.string()));
        QDir userDir(QString::fromStdString(userDbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        installDir.setNameFilters(filters);
        userDir.setNameFilters(filters);

        QFileInfoList installFileList = installDir.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : installFileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), false);
            EmuDb* otherDb = manager->GetDbFromFilePath(fileInfo.absoluteFilePath().toStdString());
            if (otherDb == nullptr) {
                AddDatabase(&db, true);
            }
        }

        QFileInfoList userFileList = userDir.entryInfoList(QDir::Files);
        for (const QFileInfo& fileInfo : userFileList) {
            EmuDb db(fileInfo.absoluteFilePath().toStdString(), false);
            EmuDb* otherDb = manager->GetDbFromFilePath(fileInfo.absoluteFilePath().toStdString());
            if (otherDb == nullptr) {
                AddDatabase(&db, true);
            }
        }
    }
}

// isTemp var exists to prevent dangling references from being set
void EmuDbComboBox::AddDatabase(EmuDb* db, bool isTemp) {
    if (db->Fail())
        return;
    QIcon icon(":/images/silk/database.png");
    std::vector<unsigned char> iconData = db->GetIcon();
    if (iconData.empty())
        iconData = EmuDb::GetDefaultIconData();
    QPixmap pixmap;
    if (pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size())))
        icon = QIcon(pixmap);

    QString fileName = QString::fromStdString(db->GetFileName());
    QString filePath = QString::fromStdString(db->GetFilePath().string());

    QString title = QString::fromStdString(db->GetTitle());
    if (title.isEmpty())
        title = fileName;

    addItem(icon, title, !isTemp ? QVariant::fromValue(reinterpret_cast<quintptr>(db)) : QVariant());
}

bool EmuDbComboBox::SetDatabase(EmuDb* db) {
    int index = findData(QVariant::fromValue(reinterpret_cast<quintptr>(db)));
    if (index != -1) {
        setCurrentIndex(index);
        return true;
    }
    return false;
}

EmuDb* EmuDbComboBox::GetSelectedDatabase() {
    if (currentIndex() == -1)
        return nullptr;
    return reinterpret_cast<EmuDb*>(currentData().value<quintptr>());
}