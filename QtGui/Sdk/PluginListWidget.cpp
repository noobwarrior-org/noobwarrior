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
// File: PluginListWidget.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "PluginListWidget.h"

#include "../Application.h"
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/PluginManager.h>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFont>
#include <QMimeData>
#include <QPalette>
#include <QPixmap>
#include <QUrl>

#include <algorithm>
#include <filesystem>

using namespace NoobWarrior;

static constexpr int RoleFileName = Qt::UserRole;
static constexpr int RoleFilePath = Qt::UserRole + 1;
static constexpr int RoleLocked   = Qt::UserRole + 2;

// Plugin::GetFileName() splits on '/' only, so on Windows it hands back the whole path. Everything
// that keys on a file name (plugins.selected, PluginManager::MountPlugins) goes through the path's
// filename() instead, so do the same here or nothing matches.
static QString FileNameOf(const Plugin::Properties& props) {
    return QString::fromStdString(props.FilePath.filename().string());
}

// plugins.selected is an array whose order is the mount order, so it has to be read by index,
// see the same note in PluginManager.cpp.
static QStringList ReadSelectionFromRegistry() {
    QStringList fileNames;
    auto selected = gApp->GetCore()->GetRegistry()->GetKeyValue<sol::table>("plugins.selected");
    if (!selected.has_value())
        return fileNames;
    for (std::size_t index = 1; index <= selected->size(); index++) {
        sol::optional<std::string> name = selected->get<sol::optional<std::string>>(index);
        if (name.has_value() && !name->empty())
            fileNames << QString::fromStdString(*name);
    }
    return fileNames;
}

PluginListWidget::PluginListWidget(Mode mode, int flags, QWidget* parent) : QListWidget(parent),
    mMode(mode), mFlags(flags)
{
    setIconSize(QSize(48, 48));
    setStyleSheet(R"(
    QListWidget::item {
        padding: 4px 4px 4px 8px;
    }
    )");
    Refresh();
}

PluginListWidget::~PluginListWidget() {

}

void PluginListWidget::Refresh() {
    clear();

    if (mMode == Mode::Manual)
        return;

    PluginManager* plgMgr = gApp->GetCore()->GetPluginManager();

    std::vector<Plugin::Properties> allProps;
    if (mFlags & NW_PRIVILEGED_PLUGINS) {
        for (const Plugin::Properties& props : plgMgr->GetPrivilegedPluginProperties())
            allProps.push_back(props);
    }
    if (mFlags & NW_NON_PRIVILEGED_PLUGINS) {
        // A plugin shipped in the install folder and one in the user folder can share a file name;
        // MountPlugins() prefers the user copy, so let the later entry win here too.
        for (const Plugin::Properties& props : plgMgr->GetPluginProperties()) {
            auto it = std::find_if(allProps.begin(), allProps.end(),
                [&props](const Plugin::Properties& other) {
                    return !other.IsPrivileged && FileNameOf(other) == FileNameOf(props);
                });
            if (it != allProps.end())
                *it = props;
            else
                allProps.push_back(props);
        }
    }

    QStringList selection = GetSelection();

    if (mMode == Mode::ShowEntriesInDir) {
        for (const Plugin::Properties& props : allProps)
            AddPlugin(props, props.IsPrivileged);
        return;
    }

    if (mMode == Mode::ShowSelected) {
        // Privileged plugins are always mounted and cannot be turned off, so they head the list
        // no matter what the user picked.
        for (const Plugin::Properties& props : allProps) {
            if (props.IsPrivileged)
                AddPlugin(props, true);
        }
        // Then the user's picks, in the order they mount in.
        for (const QString& fileName : selection) {
            for (const Plugin::Properties& props : allProps) {
                if (!props.IsPrivileged && FileNameOf(props) == fileName) {
                    AddPlugin(props);
                    break;
                }
            }
        }
        return;
    }

    if (mMode == Mode::ShowNotSelected) {
        for (const Plugin::Properties& props : allProps) {
            if (props.IsPrivileged)
                continue;
            if (!selection.contains(FileNameOf(props)))
                AddPlugin(props);
        }
    }
}

void PluginListWidget::AddPlugin(const Plugin::Properties& props, bool locked) {
    QIcon icon;
    if (!props.IconData.empty()) {
        QPixmap pixmap;
        if (pixmap.loadFromData(props.IconData.data(), static_cast<uint>(props.IconData.size())))
            icon = QIcon(pixmap);
    }
    if (icon.isNull())
        icon = QIcon(":/images/silk/plugin.png");

    QString fileName = FileNameOf(props);
    QString filePath = QString::fromStdString(props.FilePath.string());

    QString title = QString::fromStdString(props.Title);
    if (title.isEmpty())
        title = fileName;
    if (!props.Version.empty())
        title += QString(" (%1)").arg(QString::fromStdString(props.Version));

    QString authors;
    for (const std::string& author : props.Authors) {
        if (!authors.isEmpty())
            authors += ", ";
        authors += QString::fromStdString(author);
    }

    QString toolTip = filePath;
    if (!authors.isEmpty())
        toolTip = QString("By %1\n%2").arg(authors, toolTip);
    if (!props.Description.empty())
        toolTip = QString("%1\n\n%2").arg(QString::fromStdString(props.Description), toolTip);
    if (locked)
        toolTip = QString("Built-in plugin, always enabled.\n\n%1").arg(toolTip);

    auto* item = new QListWidgetItem(icon, title, this);
    item->setData(RoleFileName, fileName);
    item->setData(RoleFilePath, filePath);
    item->setData(RoleLocked, locked);
    item->setToolTip(toolTip);

    // Built-in plugins can be highlighted, clicking one has to give some feedback, but never
    // dragged around or moved out of the list. Greyed-out italics is the cue that they aren't the
    // user's to move; the button that would move them greys out to match.
    if (locked) {
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setForeground(palette().brush(QPalette::Disabled, QPalette::Text));
        QFont font = item->font();
        font.setItalic(true);
        item->setFont(font);
    }
}

void PluginListWidget::SetSelection(const QStringList& fileNames) {
    mHasSelectionOverride = true;
    mSelection = fileNames;
}

QStringList PluginListWidget::GetSelection() const {
    return mHasSelectionOverride ? mSelection : ReadSelectionFromRegistry();
}

QStringList PluginListWidget::GetFileNames() const {
    QStringList fileNames;
    for (int i = 0; i < count(); i++) {
        QListWidgetItem* it = item(i);
        if (!IsItemLocked(it))
            fileNames << GetItemFileName(it);
    }
    return fileNames;
}

QStringList PluginListWidget::GetHighlightedFileNames() const {
    QStringList fileNames;
    for (QListWidgetItem* it : selectedItems()) {
        if (!IsItemLocked(it))
            fileNames << GetItemFileName(it);
    }
    return fileNames;
}

QString PluginListWidget::GetItemFileName(const QListWidgetItem* item) {
    return item != nullptr ? item->data(RoleFileName).toString() : QString();
}

QString PluginListWidget::GetItemFilePath(const QListWidgetItem* item) {
    return item != nullptr ? item->data(RoleFilePath).toString() : QString();
}

bool PluginListWidget::IsItemLocked(const QListWidgetItem* item) {
    return item != nullptr && item->data(RoleLocked).toBool();
}

// A plugin is either a .zip archive or a directory holding a plugin.lua, so both are droppable.
static QStringList ExtractPluginPaths(const QMimeData* mime) {
    QStringList paths;
    if (mime == nullptr || !mime->hasUrls())
        return paths;
    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile())
            continue;
        QString path = url.toLocalFile();
        QFileInfo info(path);
        if (info.isDir() || path.endsWith(".zip", Qt::CaseInsensitive))
            paths << path;
    }
    return paths;
}

void PluginListWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (!ExtractPluginPaths(event->mimeData()).isEmpty()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dragEnterEvent(event);
}

void PluginListWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (!ExtractPluginPaths(event->mimeData()).isEmpty()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dragMoveEvent(event);
}

void PluginListWidget::dropEvent(QDropEvent* event) {
    QStringList paths = ExtractPluginPaths(event->mimeData());
    if (!paths.isEmpty()) {
        emit filesDropped(paths);
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QListWidget::dropEvent(event);
}
