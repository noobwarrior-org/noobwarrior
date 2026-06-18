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
// File: ItemBrowserPage.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include "ItemWidget.h"

#include <QListWidget>
#include <QAbstractListModel>
#include <QPointer>

class QMediaPlayer;
class QAudioOutput;
class QTemporaryFile;
class QMouseEvent;

#include <string>
#include <functional>
#include <tuple>
#include <map>
#include <utility>

namespace NoobWarrior {
class ItemListModel : public QAbstractListModel {
public:

};

class ItemListWidget : public QListWidget {
public:
    struct PopulateOptions {
        EmuDb* Database { nullptr };
        ItemType ItemType { ItemType::Asset };
        Roblox::AssetType AssetType { Roblox::AssetType::None };
        /* Offset and Limit have no effect if EnforceLimit is set to false */
        int Offset { 0 };
        int Limit { 100 };
        bool EnforceLimit { false };
        /* Query has no effect if it is set to blank */
        std::string Query { "" };
    };

    ItemListWidget(QWidget *parent = nullptr, EmuDb* db = nullptr);
    void Populate(const PopulateOptions options);
    // Empties the list safely: stops inline playback and clears the internal item map alongside the
    // widget items. Prefer this over QListWidget::clear(), which would leave the map dangling.
    void Clear();
    // Counts all rows matching the options' filters (query + asset type), ignoring paging. Used to
    // compute how many pages a paged view spans.
    int CountItems(const PopulateOptions &options);
    bool Add(ItemType type, int64_t id);
    bool Remove(ItemType type, int64_t id);
    bool IsItemInList(ItemType type, int64_t id);
    ItemWidget* GetItemWidget(ItemType type, int64_t id);
    std::vector<std::pair<ItemType, int64_t>> GetItems();

    void SetOnDoubleClick(const std::function<void(ItemWidget*)> func);
    void SetOnContextMenuShown(const std::function<void(QMenu*, ItemWidget*)> func);

    // Enables selecting more than one item at once (Ctrl/Shift-click and rubber-band). Off by
    // default, preserving the single-selection behaviour the item pickers rely on.
    void SetMultiSelect(bool enabled);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void InitWidgets();
    void ShowContextMenu(QPoint point);
    void DownloadSelectedAssetData();

    // The on-icon play badge's hit rectangle (in viewport coordinates) for a playable item.
    QRect PlayBadgeRect(QListWidgetItem *item);
    // Plays the item's audio inline, or stops it if that same item is already playing (click to toggle).
    void TogglePlayItem(ItemWidget *item);
    // Stops inline playback and forgets the active item. Called when the list is repopulated or the
    // playing item is removed, so audio doesn't outlive its widget.
    void StopPlayback();

    // Snapshots the selected items into the shared (process-wide) clipboard. When cut is true the
    // originals are removed from their source database once they are pasted elsewhere.
    void CopySelectedItems(bool cut);
    // Imports whatever is on the clipboard into this widget's database, prompting before
    // overwriting any id that already exists. Completes a pending cut by deleting the originals.
    void PasteItems();
    // Prompts for a new name and renames the item (updates the DB and the displayed label).
    void RenameItem(ItemWidget *item);
    // Removes the faded "pending cut" appearance from the currently-cut items (if their source
    // widget is still alive). Called before a new copy/cut replaces the clipboard, or once a cut
    // completes.
    static void ClearCutAppearance();

    PopulateOptions mLastOptions;
    std::function<void(ItemWidget*)> mOnDoubleClick;
    std::function<void(QMenu*, ItemWidget*)> mOnContextMenuShown;
    std::map<std::pair<ItemType, int64_t>, ItemWidget*> mItems;

    // Inline audio playback for the icon play badges. Created lazily on first use.
    QMediaPlayer* mPlayer { nullptr };
    QAudioOutput* mAudioOutput { nullptr };
    QTemporaryFile* mPlayTempFile { nullptr };
    std::pair<ItemType, int64_t> mPlayingKey { ItemType::Asset, -1 };
    // True between a press on a play badge and its release, so the intervening drag doesn't start a
    // rubber-band selection.
    bool mBadgePressActive { false };

    // Process-wide clipboard shared by every list, so an item copied/cut in one database's list can
    // be pasted into another's. A cut remembers its source so the originals can be deleted on paste.
    static std::vector<EmuDb::ItemSnapshot> sClipboard;
    static bool sClipboardIsCut;
    static EmuDb* sCutSourceDb;
    static QPointer<ItemListWidget> sCutSourceWidget;
};
}