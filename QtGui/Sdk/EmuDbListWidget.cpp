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
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/Url.h>
#include <QDir>
#include <QFont>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

using namespace NoobWarrior;

static QIcon DatabaseIcon(const std::vector<unsigned char> &iconData) {
    QPixmap pixmap;
    if (!iconData.empty() &&
        pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size())))
        return QIcon(pixmap);

    const std::vector<unsigned char> fallback = EmuDb::GetDefaultIconData();
    if (pixmap.loadFromData(fallback.data(), static_cast<uint>(fallback.size())))
        return QIcon(pixmap);
    return QIcon(":/images/silk/database.png");
}

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
        PluginManager *plugins = gApp->GetCore()->GetPluginManager();

        // Databases a plugin offers but that nobody has opted into yet. Listed first because they
        // have no file in the databases folder for the loops below to find.
        for (const Plugin::DeclaredDatabase &offered : plugins->GetOfferedDatabases()) {
            Plugin *owner = plugins->GetPluginFromIdentifier(offered.OwnerIdentifier);
            // The database is not mounted, so read its title and icon straight out of the file. A
            // zip plugin nobody has mounted from has nothing on disk yet; that falls back to the
            // file name.
            std::filesystem::path path = plugins->ResolveDeclaredDatabasePath(offered);
            QString title = QString::fromStdString(EmuDb::ProbeTitle(path));
            if (title.isEmpty())
                title = QString::fromStdString(Url(offered.SourceUrl).GetFileName());
            QString pluginTitle = QString::fromStdString(
                owner != nullptr ? owner->GetProperties().Title : offered.OwnerIdentifier);
            AddOfferedDatabase(title, QString::fromStdString(offered.SourceUrl), pluginTitle,
                               EmuDb::ProbeIcon(path));
        }

        std::filesystem::path installDbPath = gApp->GetCore()->GetInstallDataDir() / NW_PATH_DATABASES;
        std::filesystem::path userDbPath = gApp->GetCore()->GetUserDataDir() / NW_PATH_DATABASES;

        QDir installDir(QString::fromStdString(installDbPath.string()));
        QDir userDir(QString::fromStdString(userDbPath.string()));
        
        QStringList filters;
        filters << "*.nwdb";
        installDir.setNameFilters(filters);
        userDir.setNameFilters(filters);

        // Was lazy so I duplicated the logic twice
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
void EmuDbListWidget::AddDatabase(EmuDb* db, bool isTemp) {
    if (db->Fail())
        return;
    QIcon icon = DatabaseIcon(db->GetIcon());

    QString fileName = QString::fromStdString(db->GetFileName());
    QString filePath = QString::fromStdString(db->GetFilePath().string());

    // A throwaway EmuDb is not mounted, so it has no ownership to look up.
    EmuDbManager *manager = gApp->GetCore()->GetEmuDbManager();
    const EmuDbManager::MountInfo *info = isTemp ? nullptr : manager->GetMountInfo(db);

    QString title = QString::fromStdString(db->GetTitle());
    if (title.isEmpty())
        title = fileName;

    auto* item = new QListWidgetItem(icon, title, this);
    if (!isTemp)
        item->setData(Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(db)));
    item->setToolTip(filePath);

    if (info == nullptr || info->OwnerPluginId.empty())
        return;

    item->setData(SourceUrlRole, QString::fromStdString(info->SourceUrl));
    item->setData(LockedRole, info->Locked);
    item->setToolTip(QString("%1\n\nProvided by plugin \"%2\"%3")
        .arg(filePath)
        .arg(QString::fromStdString(info->OwnerPluginId))
        .arg(info->Locked ? "\nThis plugin requires it, so it cannot be removed here." : ""));

    if (info->Locked) {
        // Same treatment PluginListWidget gives privileged plugins: visible, but not the user's to
        // move or delete.
        QFont itemFont = item->font();
        itemFont.setItalic(true);
        item->setFont(itemFont);
        item->setForeground(palette().brush(QPalette::Disabled, QPalette::Text));
        item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
    }
}

void EmuDbListWidget::AddOfferedDatabase(const QString& title, const QString& sourceUrl,
                                         const QString& pluginTitle,
                                         const std::vector<unsigned char>& iconData) {
    auto* item = new QListWidgetItem(DatabaseIcon(iconData), title, this);
    // No Qt::UserRole: nothing is mounted yet, so there is no EmuDb to point at. DatabaseDialog
    // mounts these through the source url instead.
    item->setData(SourceUrlRole, sourceUrl);
    item->setToolTip(QString("%1\n\nOffered by plugin \"%2\"").arg(sourceUrl).arg(pluginTitle));
}

EmuDb* EmuDbListWidget::GetSelectedDatabase() {
    QListWidgetItem *item = currentItem();
    if (item != nullptr)
        return reinterpret_cast<EmuDb*>(item->data(Qt::UserRole).value<quintptr>());
    return nullptr;
}

static QStringList ExtractNwdbPaths(const QMimeData* mime) {
    QStringList paths;
    if (mime == nullptr || !mime->hasUrls())
        return paths;
    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile())
            continue;
        QString path = url.toLocalFile();
        if (path.endsWith(".nwdb", Qt::CaseInsensitive))
            paths << path;
    }
    return paths;
}

void EmuDbListWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (!ExtractNwdbPaths(event->mimeData()).isEmpty()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void EmuDbListWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (!ExtractNwdbPaths(event->mimeData()).isEmpty()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void EmuDbListWidget::dropEvent(QDropEvent* event) {
    QStringList paths = ExtractNwdbPaths(event->mimeData());
    if (!paths.isEmpty()) {
        emit filesDropped(paths);
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dropEvent(event);
}

QList<EmuDb*> EmuDbListWidget::GetSelectedDatabases() {
    QList<EmuDb*> dbs;
    QList<QListWidgetItem*> items = selectedItems();
    for (auto *item : items) {
        dbs.push_back(reinterpret_cast<EmuDb*>(item->data(Qt::UserRole).value<quintptr>()));
    }
    return dbs;
}
