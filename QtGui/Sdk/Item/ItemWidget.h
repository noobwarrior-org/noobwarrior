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
// File: ItemWidget.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include "Sdk/Sdk.h"

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>

#include <QListWidgetItem>
#include <QPixmap>
#include <QString>

namespace NoobWarrior {
class ItemWidget : public QListWidgetItem {
public:
    ItemWidget(EmuDb *db, NoobWarrior::ItemType type, int64_t id, QListWidget *listview = nullptr);
    // A DB-less item (mDb == nullptr): name + icon supplied directly rather than read from an EmuDb (used
    // by the avatar picker for a remote catalog item). A non-empty originDomain tags the federated master
    // (and db) it came from, shown as a small badge + tooltip.
    ItemWidget(int64_t id, const QString& name, const QPixmap& icon,
               const QString& originDomain = QString(), const QString& originDb = QString(),
               QListWidget *listview = nullptr);

    void Configure();
    NoobWarrior::ItemType GetType();
    int64_t GetId();
    // The federated master + database this item came from (empty for a local / same-server item). Kept for
    // a preview dialog, not shown inline.
    QString GetOriginDomain() const { return mOriginDomain; }
    QString GetOriginDb() const { return mOriginDb; }

    // Re-reads the item's name, type, icon and play badge from the database and rebuilds its display
    // (used to refresh an item in place, e.g. after it's overwritten by a paste).
    void Reload();

    // True for Audio assets, which carry a clickable play/pause badge on their icon.
    bool IsPlayable() const { return mPlayable; }
    // Swaps the icon's badge between a play triangle (paused/stopped) and a pause glyph (playing).
    void SetPlaying(bool playing);

    // Replaces the icon in place (used to fill in a DB-less remote item's thumbnail once it arrives async).
    void SetRemoteIcon(const QPixmap& icon);

    // Greys the label and fades the icon to mark the item as part of a pending Cut (and undoes it).
    void SetCut(bool cut);
    // Re-reads the item's Name from the database and refreshes the displayed label (used after Rename).
    void RefreshName();
private:
    // Composites a media badge (play triangle or pause bars) onto the bottom-right of an icon.
    static void Asset_DrawMediaBadge(QPixmap &pixmap, bool playing);
    // Applies the current icon (faded when cut) and label colour to reflect mCut.
    void ApplyAppearance();

    bool mPlayable { false };
    bool mCut { false };
    // The icon without any badge, cached so the badge can be redrawn when playback state changes.
    QPixmap mBasePixmap;
    // The composed icon (thumbnail + any badge), cached so it can be faded/restored for cut state.
    QPixmap mIconPixmap;

    // Federated origin of a DB-less item (empty otherwise): the master domain + database it came from.
    QString mOriginDomain;
    QString mOriginDb;

    EmuDb* mDb;
    NoobWarrior::ItemType mType;
    int64_t mId;
};
}
