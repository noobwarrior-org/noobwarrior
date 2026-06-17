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
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstring>

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

static QString DetectAssetExtension(const std::vector<unsigned char> &data) {
    auto starts = [&](const char *sig, size_t len) {
        if (data.size() < len)
            return false;
        return std::memcmp(data.data(), sig, len) == 0;
    };

    if (starts("<roblox!", 8))
        return "rbxm";
    if (starts("<roblox", 7))
        return "rbxmx";
    if (starts("version ", 8))
        return "mesh";

    if (starts("\x89PNG\r\n\x1a\n", 8))
        return "png";
    if (starts("\xff\xd8\xff", 3))
        return "jpg";
    if (starts("GIF87a", 6) || starts("GIF89a", 6))
        return "gif";
    if (starts("BM", 2))
        return "bmp";
    if (data.size() >= 12 && starts("RIFF", 4) && std::memcmp(data.data() + 8, "WEBP", 4) == 0)
        return "webp";
    if (data.size() >= 12 && starts("RIFF", 4) && std::memcmp(data.data() + 8, "WAVE", 4) == 0)
        return "wav";
    if (starts("OggS", 4))
        return "ogg";
    if (starts("ID3", 3) || starts("\xff\xfb", 2) || starts("\xff\xf3", 2) || starts("\xff\xf2", 2))
        return "mp3";
    if (starts("fLaC", 4))
        return "flac";
    if (starts("\xabKTX 11\xbb\r\n\x1a\n", 12))
        return "ktx";
    if (starts("DDS ", 4))
        return "dds";
    if (starts("Kaydara FBX Binary", 18))
        return "fbx";
    if (starts("%PDF", 4))
        return "pdf";
    if (starts("PK\x03\x04", 4))
        return "zip";

    return "bin";
}

static QString SanitizeFileName(const QString &name) {
    QString result;
    for (QChar c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|' || c.unicode() < 0x20)
            result += '_';
        else
            result += c;
    }
    result = result.trimmed();
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

        if (item && item->GetType() == ItemType::Asset) {
            QAction* download = menu->addAction(QIcon(":/images/silk/disk.png"), "Download Asset Data");
            connect(download, &QAction::triggered, [this]() {
                DownloadSelectedAssetData();
            });
        }

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
    setResizeMode(QListView::Adjust);
    setSpacing(6);
    setGridSize(QSize(110, 112));

    connect(this, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        auto *contentItem = dynamic_cast<ItemWidget*>(item);
        if (contentItem) {
            mOnDoubleClick(contentItem);
        }
    });
}

void ItemListWidget::DownloadSelectedAssetData() {
    if (mLastOptions.Database == nullptr)
        return;

    std::vector<ItemWidget*> assets;
    for (QListWidgetItem *item : selectedItems()) {
        if (auto *itemWidget = dynamic_cast<ItemWidget*>(item)) {
            if (itemWidget->GetType() == ItemType::Asset)
                assets.push_back(itemWidget);
        }
    }

    if (assets.empty())
        return;

    QStringList failures;
    int saved = 0;
    QString lastDir;

    for (ItemWidget *itemWidget : assets) {
        int64_t id = itemWidget->GetId();

        std::vector<unsigned char> data;
        SqlDb::Response res = mLastOptions.Database->RetrieveAssetData(id, 0, &data);
        if (res != SqlDb::Response::Success || data.empty()) {
            failures << QString::number(id);
            continue;
        }

        QString baseName;
        Statement nameStmt = mLastOptions.Database->PrepareStatement("SELECT Name FROM Asset WHERE Id = ?;");
        nameStmt.Bind(1, id);
        if (nameStmt.Step() == SQLITE_ROW)
            baseName = SanitizeFileName(QString::fromStdString(nameStmt.GetStringFromColumnIndex(0)));
        if (baseName.isEmpty())
            baseName = QString::number(id);

        QString ext = DetectAssetExtension(data);
        QString suggested = QString("%1_%2.%3").arg(baseName).arg(id).arg(ext);
        QString startPath = lastDir.isEmpty() ? suggested : QDir(lastDir).filePath(suggested);

        QString fullPath = QFileDialog::getSaveFileName(this,
            QString("Save Asset %1").arg(id), startPath, "All Files (*)");
        if (fullPath.isEmpty())
            continue;

        lastDir = QFileInfo(fullPath).absolutePath();

        QFile file(fullPath);
        if (!file.open(QIODevice::WriteOnly)) {
            failures << QString::number(id);
            continue;
        }
        qint64 written = file.write(reinterpret_cast<const char*>(data.data()), static_cast<qint64>(data.size()));
        file.close();
        if (written != static_cast<qint64>(data.size())) {
            failures << QString::number(id);
            continue;
        }
        saved++;
    }

    if (!failures.isEmpty()) {
        QMessageBox::warning(this, "Download Asset Data",
            QString("Saved %1 asset(s). Failed to download %2 asset(s): %3")
                .arg(saved)
                .arg(failures.size())
                .arg(failures.join(", ")));
    }
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