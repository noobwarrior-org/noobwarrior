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
// File: BrowserItem.cpp
// Started by: Hattozo
// Started on: 2/14/2026
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "BrowserItem.h"
#include "Sdk/Sdk.h"

#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

using namespace NoobWarrior;

BrowserItem::BrowserItem(EmuDb *db, const std::string &tableName, int id, int snapshot, QListWidget *listview)  :
    QListWidgetItem(listview)
{
    std::string name;

    Statement stmt = db->PrepareStatement(std::format("SELECT Name FROM {} WHERE Id = ? {}", tableName, snapshot > 0 ? "AND Snapshot = ?" : "ORDER BY Snapshot DESC LIMIT 1"));
    if (stmt.Fail()) {
        Out("BrowserItem", "Failed to retrieve name for ID {}", id);
        return;
    }
    stmt.Bind(1, id);
    if (snapshot > 0)
        stmt.Bind(2, snapshot);
    if (stmt.Step() == SQLITE_ROW) {
        name = stmt.GetStringFromColumnIndex(0);
    }

    setText(QString("%1\n(%2)").arg(QString::fromStdString(name), QString::number(id)));

    std::vector<unsigned char> imageData = db->RetrieveImageData(tableName, id, snapshot);
    if (!imageData.empty()) {
        QImage image;
        image.loadFromData(imageData);

        QPixmap pixmap = QPixmap::fromImage(image);

        setIcon(QIcon(pixmap));
    }
}

void Configure(Sdk *editor) {
    // auto editDialog = ContentEditorDialog<T>(editor, mRecord.Id);
    // editDialog.exec();
}