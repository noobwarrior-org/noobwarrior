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
// File: EngineTreeWidget.cpp
// Started by: Hattozo
// Started on: 5/29/2026
// Description:
#include "EngineTreeWidget.h"

using namespace NoobWarrior;

EngineTreeWidget::EngineTreeWidget() {

}

void EngineTreeWidget::InitWidgets() {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto engineModel = new QStandardItemModel(this);
    engineModel->setColumnCount(6);
    engineModel->setHorizontalHeaderLabels({"", "Installed", "Side", "Version", "Hash", "Date"});
    setModel(engineModel);
    setColumnHidden(0, true); // for some odd reason, the first column cannot be reordered. so we just make it blank and hide it. good job!
}