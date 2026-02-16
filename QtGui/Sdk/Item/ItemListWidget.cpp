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
#include "ItemListWidget.h"
#include "Browser/BrowserItem.h"

#include <QMenu>
#include <QMessageBox>

using namespace NoobWarrior;

ItemListWidget::ItemListWidget(QWidget *parent) : QListWidget(parent)
{
    InitWidgets();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &ItemListWidget::ShowContextMenu);
}

void ItemListWidget::Refresh() {

}

void ItemListWidget::Populate(const PopulateOptions options) {
    clear();
    std::string tableName = GetTableNameFromItemType(options.ItemType);

    Statement stmt = options.Database->PrepareStatement("SELECT Id, Name FROM " + tableName + ";");

    while (stmt.Step() == SQLITE_ROW) {
        
    }
}

void ItemListWidget::InitWidgets() {
    // optimizations to make it less laggy
    setUniformItemSizes(true);
    setLayoutMode(QListView::Batched);

    setMovement(QListView::Static);
    setViewMode(QListView::IconMode);
    setIconSize(QSize(64, 64));
    setWordWrap(true);

    connect(this, &QListWidget::itemDoubleClicked, this, [](QListWidgetItem *item) {
        // auto *contentItem = dynamic_cast<BrowserItem*>(item);
        // contentItem->Configure(editor);
    });
}

void ItemListWidget::ShowContextMenu(QPoint point) {
    if (selectedItems().empty())
        return;

    QPoint globalPos = mapToGlobal(point);

    QMenu myMenu;
    QAction* config = myMenu.addAction(QIcon(":/images/silk/cog.png"), "Configure Item");
    QAction* del = myMenu.addAction(QIcon(":/images/silk/cross.png"), "Delete Item");

    connect(config, &QAction::triggered, [this]() {
        QListWidgetItem *item = currentItem();
        auto *contentItem = dynamic_cast<BrowserItem*>(item);
        contentItem->Configure();
    });

    connect(del, &QAction::triggered, [this]() {
        QMessageBox::StandardButton button = QMessageBox::warning(this, "Delete Item", "Are you sure you want to delete this item?", QMessageBox::Yes | QMessageBox::No);
        if (button != QMessageBox::Yes)
            return;

        for (QListWidgetItem *item : selectedItems()) {
            QMessageBox::warning(this, "Notice", "Deleting items doesn't actually work for now lmao. The item has temporarily disappeared as a placeholder.");
            delete takeItem(row(item));
        }
    });

    myMenu.exec(globalPos);
}