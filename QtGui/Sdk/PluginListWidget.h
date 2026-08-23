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
// File: PluginListWidget.h
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#pragma once
#include <QListWidget>
#include <QStringList>
#include <NoobWarrior/Plugin.h>

namespace NoobWarrior {
// "Selected" here always means "listed in the plugins.selected registry key", i.e. the plugin is
// enabled and gets mounted on startup. The Qt sense of selected (the rows the user highlighted) is
// called "highlighted" throughout this class so the two never get confused.
class PluginListWidget : public QListWidget {
    Q_OBJECT
public:
    enum class Mode {
        ShowEntriesInDir, // Shows every plugin found in the plugins folders, selected or not
        ShowSelected, // Shows only the selected plugins, in the order they are mounted in
        ShowNotSelected, // Shows the plugins that aren't selected
        Manual // Shows nothing, you add stuff manually
    };

    PluginListWidget(Mode mode = Mode::ShowEntriesInDir,
                     int flags = NW_NON_PRIVILEGED_PLUGINS,
                     QWidget* parent = nullptr);
    ~PluginListWidget();

    void Refresh();
    void AddPlugin(const Plugin::Properties& props, bool locked = false);

    /**
     * @brief Overrides the list of selected plugin file names that ShowSelected/ShowNotSelected
     * filter against. A dialog that stages changes before writing them (PluginDialog) hands its
     * working copy over here; leave it unset and the widget reads plugins.selected straight out of
     * the registry instead.
     */
    void SetSelection(const QStringList& fileNames);
    QStringList GetSelection() const;

    /**
     * @brief File names of the items in the list, in row order. Locked (privileged) items are left
     * out: they are shown for information only and are never part of plugins.selected.
     */
    QStringList GetFileNames() const;

    /**
     * @brief File names of the items the user highlighted, in row order. Locked items are left out.
     */
    QStringList GetHighlightedFileNames() const;

    static QString GetItemFileName(const QListWidgetItem* item);
    static QString GetItemFilePath(const QListWidgetItem* item);
    static bool IsItemLocked(const QListWidgetItem* item);
signals:
    void filesDropped(const QStringList& filePaths);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
private:
    Mode mMode;
    int mFlags;
    bool mHasSelectionOverride = false;
    QStringList mSelection;
};
}
