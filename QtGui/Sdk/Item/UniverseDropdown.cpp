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
#include <qnamespace.h>

using namespace NoobWarrior;

UniverseDropdown::UniverseDropdown(QWidget *parent) : QTreeWidget(parent) {
    setSelectionMode(QAbstractItemView::SingleSelection);
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
        bool universeHasPlace = false;

        std::vector<unsigned char> iconData = db->RetrieveImageData("Universe", id);
        QPixmap pixmap;
        pixmap.loadFromData(iconData.data(), static_cast<uint>(iconData.size()));
        QIcon icon = pixmap;

        auto *universeItem = new QTreeWidgetItem(this);
        universeItem->setData(0, Qt::UserRole, QVariant::fromValue<int64_t>(startPlaceId));
        universeItem->setData(0, Qt::UserRole + 1, 0); // a flag that tells us if there is something unusual about this universe
        universeItem->setText(0, name);
        universeItem->setIcon(0, icon);

        Statement getPlaceIdStmt = db->PrepareStatement("SELECT PlaceId FROM UniversePlace WHERE Id = ?;");
        getPlaceIdStmt.Bind(1, id);
        while (getPlaceIdStmt.Step() == SQLITE_ROW) {
            universeHasPlace = true;
            int64_t placeId = getPlaceIdStmt.GetInt64FromColumnIndex(0);

            auto *placeItem = new QTreeWidgetItem();
            placeItem->setData(0, Qt::UserRole, QVariant::fromValue<int64_t>(placeId));
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

        if (!universeHasPlace) { // if this universe doesn't have any places at all
            universeItem->setData(0, Qt::UserRole + 1, 1);
        } else if (universePlacesStmt.IsColumnIndexNull(1)) { // if this universe doesn't have a start place
            universeItem->setData(0, Qt::UserRole + 1, 2);
        }
    }
}

std::optional<int64_t> UniverseDropdown::GetSelectedPlaceId() {
    QList<QTreeWidgetItem*> items = selectedItems();
    if (items.size() <= 0) {
        return std::nullopt;
    }

    QTreeWidgetItem* item = items.at(0);
    int64_t placeId = item->data(0, Qt::UserRole).toLongLong();
    return placeId;
}
