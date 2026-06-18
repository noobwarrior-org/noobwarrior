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

std::vector<EmuDb::ItemSnapshot> ItemListWidget::sClipboard;
bool ItemListWidget::sClipboardIsCut = false;
EmuDb* ItemListWidget::sCutSourceDb = nullptr;
QPointer<ItemListWidget> ItemListWidget::sCutSourceWidget = nullptr;

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

        menu->addSeparator();
        QAction* copy = menu->addAction(QIcon(":/images/silk/page_copy.png"), "Copy");
        QAction* cut = menu->addAction(QIcon(":/images/silk/cut.png"), "Cut");
        QAction* paste = menu->addAction(QIcon(":/images/silk/paste_plain.png"), "Paste");
        paste->setEnabled(!sClipboard.empty());

        connect(copy, &QAction::triggered, [this]() { CopySelectedItems(false); });
        connect(cut, &QAction::triggered, [this]() { CopySelectedItems(true); });
        connect(paste, &QAction::triggered, [this]() { PasteItems(); });

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
    if (options.Database == nullptr)
        return;
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
    // Page the results when a limit is enforced. Offset/Limit are integers from code, so they're
    // safe to inline. Order by Id for a stable page ordering across queries.
    if (options.EnforceLimit) {
        stmtStr += " ORDER BY Id LIMIT " + std::to_string(options.Limit) +
                   " OFFSET " + std::to_string(options.Offset);
    }
    stmtStr += ";";
    Statement stmt = options.Database->PrepareStatement(stmtStr);
    if (!options.Query.empty())
        stmt.Bind(1, "%" + EscapeLike(options.Query) + "%");

    while (stmt.Step() == SQLITE_ROW) {
        Add(options.ItemType, stmt.GetInt64FromColumnIndex(0));
    }
}

int ItemListWidget::CountItems(const PopulateOptions &options) {
    if (options.Database == nullptr)
        return 0;
    std::string tableName = GetTableNameFromItemType(options.ItemType);

    // Mirror Populate's filters (without the paging clause) to count every matching row.
    std::string stmtStr = "SELECT COUNT(*) FROM \"" + tableName + "\"";
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

    if (stmt.Step() == SQLITE_ROW)
        return stmt.GetIntFromColumnIndex(0);
    return 0;
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

void ItemListWidget::SetMultiSelect(bool enabled) {
    setSelectionMode(enabled ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
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
    QPoint globalPos = mapToGlobal(point);
    QMenu menu;

    if (!selectedItems().empty()) {
        QListWidgetItem *item = currentItem();
        auto *itemWidget = dynamic_cast<ItemWidget*>(item);
        mOnContextMenuShown(&menu, itemWidget);
    } else if (!sClipboard.empty()) {
        // Nothing is selected, but there's something to paste: offer just that.
        QAction* paste = menu.addAction(QIcon(":/images/silk/paste_plain.png"), "Paste");
        connect(paste, &QAction::triggered, [this]() { PasteItems(); });
    } else {
        return;
    }

    menu.exec(globalPos);
}

void ItemListWidget::CopySelectedItems(bool cut) {
    if (mLastOptions.Database == nullptr)
        return;

    std::vector<EmuDb::ItemSnapshot> snapshots;
    QStringList failures;
    for (QListWidgetItem *item : selectedItems()) {
        auto *itemWidget = dynamic_cast<ItemWidget*>(item);
        if (!itemWidget)
            continue;

        EmuDb::ItemSnapshot snap;
        SqlDb::Response res = mLastOptions.Database->ExportItem(itemWidget->GetType(), itemWidget->GetId(), &snap);
        if (res == SqlDb::Response::Success)
            snapshots.push_back(std::move(snap));
        else
            failures << QString::number(itemWidget->GetId());
    }

    if (snapshots.empty()) {
        if (!failures.isEmpty())
            QMessageBox::warning(this, cut ? "Cut" : "Copy", "Failed to read the selected item(s).");
        return;
    }

    sClipboard = std::move(snapshots);
    sClipboardIsCut = cut;
    sCutSourceDb = cut ? mLastOptions.Database : nullptr;
    sCutSourceWidget = cut ? this : nullptr;

    if (!failures.isEmpty()) {
        QMessageBox::warning(this, cut ? "Cut" : "Copy",
            QString("%1 item(s) ready to paste. Failed to read: %2")
                .arg(sClipboard.size())
                .arg(failures.join(", ")));
    }
}

void ItemListWidget::PasteItems() {
    if (mLastOptions.Database == nullptr || sClipboard.empty())
        return;

    EmuDb* target = mLastOptions.Database;
    // A cut that lands back in its own source database is just a move-in-place: there's nothing to
    // relocate, so treat it as a plain paste and don't delete the "originals" we just wrote.
    bool performCutDeletion = sClipboardIsCut && sCutSourceDb != nullptr && sCutSourceDb != target;

    // Remembered across items so "Yes to All" / "No to All" apply to the rest of the batch.
    bool overwriteAll = false;
    bool skipAll = false;

    QStringList failures;
    int pasted = 0;

    for (const EmuDb::ItemSnapshot &snap : sClipboard) {
        bool overwrite = false;
        if (target->DoesItemExist(snap.Type, snap.Id)) {
            if (skipAll)
                continue;
            if (overwriteAll) {
                overwrite = true;
            } else {
                QMessageBox::StandardButton answer = QMessageBox::question(this, "Paste",
                    QString("An item with id %1 already exists in this database. Overwrite it?").arg(snap.Id),
                    QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll,
                    QMessageBox::No);
                switch (answer) {
                case QMessageBox::YesToAll: overwriteAll = true; [[fallthrough]];
                case QMessageBox::Yes:      overwrite = true; break;
                case QMessageBox::NoToAll:  skipAll = true; [[fallthrough]];
                default:                    continue; // No: skip this item
                }
            }
        }

        SqlDb::Response res = target->ImportItem(snap, overwrite);
        if (res != SqlDb::Response::Success) {
            failures << QString::number(snap.Id);
            continue;
        }

        // Reflect the paste in this list: refresh an overwritten row, add a brand-new one.
        if (IsItemInList(snap.Type, snap.Id))
            Remove(snap.Type, snap.Id);
        Add(snap.Type, snap.Id);
        pasted++;
    }

    // Complete a cut by removing the originals from their source database (and its list, if alive).
    if (performCutDeletion && pasted > 0) {
        for (const EmuDb::ItemSnapshot &snap : sClipboard) {
            if (sCutSourceDb->DeleteItem(snap.Type, snap.Id) == SqlDb::Response::Success &&
                sCutSourceWidget && sCutSourceWidget != this) {
                sCutSourceWidget->Remove(snap.Type, snap.Id);
            }
        }
    }

    // A cut is one-shot: once moved, clear it. A copy stays on the clipboard for repeated pasting.
    if (sClipboardIsCut) {
        sClipboard.clear();
        sClipboardIsCut = false;
        sCutSourceDb = nullptr;
        sCutSourceWidget = nullptr;
    }

    if (!failures.isEmpty()) {
        QMessageBox::warning(this, "Paste",
            QString("Pasted %1 item(s). Failed to paste %2 item(s): %3")
                .arg(pasted)
                .arg(failures.size())
                .arg(failures.join(", ")));
    }
}