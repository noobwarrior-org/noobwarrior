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
#include "ItemWidget.h"

#include <QMenu>
#include <QMessageBox>

using namespace NoobWarrior;

static std::string EscapeLike(const std::string &input) {
    std::string result;
    for (char c : input) {
        if (c == '%' || c == '_' || c == '\\')
            result += '\\';
        result += c;
    }
    return result;
}

ItemListWidget::ItemListWidget(QWidget *parent) : QListWidget(parent)
{
    mOnDoubleClick = [](ItemWidget* item) {
        item->Configure();
    };
    mOnContextMenuShown = [this](QMenu* menu, ItemWidget* item) {
        QAction* config = menu->addAction(QIcon(":/images/silk/cog.png"), "Configure Item");
        QAction* del = menu->addAction(QIcon(":/images/silk/cross.png"), "Delete Item");

        connect(config, &QAction::triggered, [this]() {
            QListWidgetItem *item = currentItem();
            auto *itemWidget = dynamic_cast<ItemWidget*>(item);
            if (itemWidget) {
                itemWidget->Configure();
            }
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
    };
    InitWidgets();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &ItemListWidget::ShowContextMenu);
}

void ItemListWidget::Populate(const PopulateOptions options) {
    clear();
    mItems.clear();
    std::string tableName = GetTableNameFromItemType(options.ItemType);

    std::string stmtStr = "SELECT Id, Name FROM " + tableName;
    bool hasWhere = false;

    if (!options.Query.empty()) {
        stmtStr += " WHERE Name LIKE ? ESCAPE '\\'";
        hasWhere = true;
    }
    if (options.ItemType == ItemType::Asset && options.AssetType != Roblox::AssetType::None) {
        stmtStr += hasWhere ? " AND " : " WHERE ";
        stmtStr += "Type = " + std::to_string(static_cast<int>(options.AssetType));
    }
    stmtStr += ";";
    Statement stmt = options.Database->PrepareStatement(stmtStr);
    if (!options.Query.empty())
        stmt.Bind(1, "%" + EscapeLike(options.Query) + "%");

    while (stmt.Step() == SQLITE_ROW) {
        Add(options.Database, options.ItemType, stmt.GetInt64FromColumnIndex(0));
    }
}

void ItemListWidget::Add(EmuDb* db, ItemType type, int64_t id) {
    auto *item = new ItemWidget(db, type, id, this);
    mItems[std::make_tuple(db, type, id)] = item;
}

void ItemListWidget::Remove(EmuDb* db, ItemType type, int64_t id) {
    auto it = mItems.find(std::make_tuple(db, type, id));
    if (it != mItems.end()) {
        delete it->second;
    }
}

bool ItemListWidget::IsItemInList(EmuDb* db, ItemType type, int64_t id) {
    auto it = mItems.find(std::make_tuple(db, type, id));
    return it != mItems.end();
}

void ItemListWidget::SetOnDoubleClick(const std::function<void(ItemWidget*)> func) {
    mOnDoubleClick = func;
}

void ItemListWidget::SetOnContextMenuShown(const std::function<void(QMenu*, ItemWidget*)> func) {
    mOnContextMenuShown = func;
}

void ItemListWidget::InitWidgets() {
    // optimizations to make it less laggy
    setUniformItemSizes(true);
    setLayoutMode(QListView::Batched);

    setMovement(QListView::Static);
    setViewMode(QListView::IconMode);
    setIconSize(QSize(64, 64));
    setWordWrap(true);

    connect(this, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        auto *contentItem = dynamic_cast<ItemWidget*>(item);
        if (contentItem) {
            mOnDoubleClick(contentItem);
        }
    });
}

void ItemListWidget::ShowContextMenu(QPoint point) {
    if (selectedItems().empty())
        return;

    QPoint globalPos = mapToGlobal(point);
    QMenu menu;

    QListWidgetItem *item = currentItem();
    auto *itemWidget = dynamic_cast<ItemWidget*>(item);
    mOnContextMenuShown(&menu, itemWidget);

    menu.exec(globalPos);
}