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
    
}

void UniverseDropdown::Populate(EmuDb* db) {
    if (db == nullptr)
        return;

    Statement stmt = db->PrepareStatement("SELECT Name, StartPlaceId FROM Universe;");
    while (stmt.Step() == SQLITE_ROW) {
        auto *item = new QTreeWidgetItem(this);
        item->setText(0, QString::fromStdString(stmt.GetStringFromColumnIndex(0)));

        int64_t startPlaceId = stmt.GetInt64FromColumnIndex(1);

        Statement stmt2 = db->PrepareStatement("SELECT PlaceId FROM UniversePlace WHERE Id = ?;");
        while (stmt2.Step() == SQLITE_ROW) {

        }
    }
}