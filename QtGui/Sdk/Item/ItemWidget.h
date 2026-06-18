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

namespace NoobWarrior {
class ItemWidget : public QListWidgetItem {
public:
    ItemWidget(EmuDb *db, NoobWarrior::ItemType type, int64_t id, QListWidget *listview = nullptr);

    void Configure();
    NoobWarrior::ItemType GetType();
    int64_t GetId();

    // True for Audio assets, which carry a clickable play/pause badge on their icon.
    bool IsPlayable() const { return mPlayable; }
    // Swaps the icon's badge between a play triangle (paused/stopped) and a pause glyph (playing).
    void SetPlaying(bool playing);
private:
    // Composites a media badge (play triangle or pause bars) onto the bottom-right of an icon.
    static void Asset_DrawMediaBadge(QPixmap &pixmap, bool playing);

    bool mPlayable { false };
    // The icon without any badge, cached so the badge can be redrawn when playback state changes.
    QPixmap mBasePixmap;

    EmuDb* mDb;
    NoobWarrior::ItemType mType;
    int64_t mId;
};
}
