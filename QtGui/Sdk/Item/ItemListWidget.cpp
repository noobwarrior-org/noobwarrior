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
#include "Sdk/Item/ItemWidget.h"

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

ItemListWidget::ItemListWidget(QWidget *parent, EmuDb* db) : QListWidget(parent)
{
    mLastOptions.Database = db;
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
            
            std::vector<ItemWidget*> toDelete;
            for (QListWidgetItem *item : selectedItems()) {
                if (auto *itemWidget = dynamic_cast<ItemWidget*>(item))
                    toDelete.push_back(itemWidget);
            }

            QStringList failures;
            for (ItemWidget *itemWidget : toDelete) {
                ItemType type = itemWidget->GetType();
                int64_t id = itemWidget->GetId();

                SqlDb::Response response = mLastOptions.Database->DeleteItem(type, id);
                if (response == SqlDb::Response::Success) {
                    Remove(type, id);
                } else {
                    failures << QString::number(id);
                }
            }

            if (!failures.isEmpty()) {
                QMessageBox::warning(this, "Delete Item",
                    QString("Failed to delete %1 item(s): %2")
                        .arg(failures.size())
                        .arg(failures.join(", ")));
            }
        });
    };
    InitWidgets();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListWidget::customContextMenuRequested, this, &ItemListWidget::ShowContextMenu);
}

void ItemListWidget::Populate(const PopulateOptions options) {
    mLastOptions = options;
    mItems.clear();
    clear();
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
        Add(options.ItemType, stmt.GetInt64FromColumnIndex(0));
    }
}

bool ItemListWidget::Add(ItemType type, int64_t id) {
    if (mLastOptions.Database == nullptr)
        return false;
    auto *item = new ItemWidget(mLastOptions.Database, type, id, this);
    mItems[{ type, id }] = item;
    return true;
}

bool ItemListWidget::Remove(ItemType type, int64_t id) {
    auto it = mItems.find({ type, id });
    if (it != mItems.end()) {
        delete it->second;
        mItems.erase(it);
        return true;
    }
    return false;
}

bool ItemListWidget::IsItemInList(ItemType type, int64_t id) {
    auto it = mItems.find({ type, id });
    return it != mItems.end();
}

ItemWidget* ItemListWidget::GetItemWidget(ItemType type, int64_t id) {
    auto it = mItems.find({ type, id });
    return it != mItems.end() ? it->second : nullptr;
}

std::vector<std::pair<ItemType, int64_t>> ItemListWidget::GetItems() {
    std::vector<std::pair<ItemType, int64_t>> items;
    for (const auto &[k, v] : mItems) {
        items.push_back(k);
    }
    return items;
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