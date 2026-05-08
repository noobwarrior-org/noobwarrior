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
// File: UniverseDropdown.cpp
// Started by: Hattozo
// Started on: 5/6/2026
// Description:
#include "UniverseDropdown.h"

using namespace NoobWarrior;

UniverseDropdown::UniverseDropdown(QWidget *parent) : QTreeWidget(parent) {
    setHeaderLabel("Name");
}

void UniverseDropdown::Populate(EmuDb* db) {
    clear();
    if (db == nullptr)
        return;

    Statement stmt = db->PrepareStatement("SELECT Id, StartPlaceId, Name FROM Universe;");
    while (stmt.Step() == SQLITE_ROW) {
        int64_t id = stmt.GetInt64FromColumnIndex(0);
        int64_t startPlaceId = stmt.GetInt64FromColumnIndex(1);
        QString name = QString::fromStdString(stmt.GetStringFromColumnIndex(2));

        auto *universeItem = new QTreeWidgetItem(this);
        universeItem->setText(0, name);

        Statement stmt2 = db->PrepareStatement("SELECT PlaceId FROM UniversePlace WHERE Id = ?;");
        stmt2.Bind(1, id);
        while (stmt2.Step() == SQLITE_ROW) {
            int64_t placeId = stmt2.GetInt64FromColumnIndex(0);

            auto *placeItem = new QTreeWidgetItem();
            universeItem->addChild(placeItem);

            Statement stmt3 = db->PrepareStatement("SELECT Name FROM Asset WHERE Id = ?;");
            stmt3.Bind(1, placeId);
            int res = stmt3.Step();

            if (res == SQLITE_ROW) {
                std::vector<unsigned char> iconData = db->RetrieveImageData("Asset", placeId);
                QPixmap pixmap;
                pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
                QIcon icon = pixmap;

                QString placeName = QString::fromStdString(stmt3.GetStringFromColumnIndex(0));
                placeItem->setText(0, placeName);
                placeItem->setIcon(0, icon);
            } else {
                placeItem->setText(0, "Missing Place");
            }
        }
    }
}