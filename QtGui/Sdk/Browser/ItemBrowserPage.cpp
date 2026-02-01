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
// File: ItemBrowserPage.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "ItemBrowserPage.h"

using namespace NoobWarrior;

ItemBrowserPage::ItemBrowserPage(QWidget *parent) : QListWidget(parent) {
    InitWidgets();
}

void ItemBrowserPage::InitWidgets() {
    setAutoFillBackground(true);
    setMovement(QListView::Static);
    setViewMode(QListView::IconMode);
    setIconSize(QSize(64, 64));
    setWordWrap(true);

    connect(this, &QListWidget::itemDoubleClicked, this, [](QListWidgetItem *item) {
        // auto *contentItem = dynamic_cast<BrowserItem*>(item);
        // contentItem->Configure(editor);
    });
}