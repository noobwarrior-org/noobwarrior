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
    setIconSize(QSize(48, 48));
    setStyleSheet(R"(
    QTreeWidget::item {
        padding: 4px 4px 4px 8px;
    }
    )");
}

void UniverseDropdown::Populate(EmuDb* db) {
    clear();
    if (db == nullptr)
        return;

    Statement universePlacesStmt = db->PrepareStatement("SELECT Id, StartPlaceId, Name FROM Universe;");
    while (universePlacesStmt.Step() == SQLITE_ROW) {
        int64_t id = universePlacesStmt.GetInt64FromColumnIndex(0);
        int64_t startPlaceId = universePlacesStmt.GetInt64FromColumnIndex(1);
        QString name = QString::fromStdString(universePlacesStmt.GetStringFromColumnIndex(2));

        std::vector<unsigned char> iconData = db->RetrieveImageData("Universe", id);
        QPixmap pixmap;
        pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
        QIcon icon = pixmap;

        auto *universeItem = new QTreeWidgetItem(this);
        universeItem->setText(0, name);
        universeItem->setIcon(0, icon);

        Statement getPlaceIdStmt = db->PrepareStatement("SELECT PlaceId FROM UniversePlace WHERE Id = ?;");
        getPlaceIdStmt.Bind(1, id);
        while (getPlaceIdStmt.Step() == SQLITE_ROW) {
            int64_t placeId = getPlaceIdStmt.GetInt64FromColumnIndex(0);

            auto *placeItem = new QTreeWidgetItem();
            universeItem->addChild(placeItem);

            Statement getPlaceNameStmt = db->PrepareStatement("SELECT Name FROM Asset WHERE Id = ?;");
            getPlaceNameStmt.Bind(1, placeId);
            int res = getPlaceNameStmt.Step();

            if (res == SQLITE_ROW) {
                std::vector<unsigned char> iconData = db->RetrieveImageData("Asset", placeId);
                QPixmap pixmap;
                pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
                QIcon icon = pixmap;

                QString placeName = QString::fromStdString(getPlaceNameStmt.GetStringFromColumnIndex(0));
                placeItem->setText(0, placeName);
                placeItem->setIcon(0, icon);
            } else {
                placeItem->setText(0, "Missing Place");
            }
        }
    }
}