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
#include "Sdk/Item/AssetDataFileType.h"

#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QAbstractButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTemporaryFile>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QUrl>
#include <QApplication>
#include <QClipboard>

#include <algorithm>

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
        QAction* copyId = menu->addAction(QIcon(":/images/silk/page_white_copy.png"), "Copy ID");

        if (item && item->GetType() == ItemType::Asset) {
            QAction* download = menu->addAction(QIcon(":/images/silk/disk.png"), "Export Asset Data");
            connect(download, &QAction::triggered, [this]() {
                DownloadSelectedAssetData();
            });
        }

        menu->addSeparator();
        QAction* copy = menu->addAction(QIcon(":/images/silk/page_copy.png"), "Copy");
        QAction* cut = menu->addAction(QIcon(":/images/silk/cut.png"), "Cut");

        menu->addSeparator();
        QAction* del = menu->addAction(QIcon(":/images/silk/cross.png"), "Delete Item");
        QAction* rename = menu->addAction(QIcon(":/images/silk/pencil.png"), "Rename");

        connect(copyId, &QAction::triggered, [this, item]() {
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(QString::number(item->GetId()));
        });

        connect(copy, &QAction::triggered, [this]() { CopySelectedItems(false); });
        connect(cut, &QAction::triggered, [this]() { CopySelectedItems(true); });

        connect(rename, &QAction::triggered, [this, item]() { RenameItem(item); });

        connect(config, &QAction::triggered, [this]() {
            QListWidgetItem *item = currentItem();
            auto *itemWidget = dynamic_cast<ItemWidget*>(item);
            if (itemWidget) {
                itemWidget->Configure();
            }
        });

        connect(del, &QAction::triggered, [this]() {
            std::vector<ItemWidget*> toDelete;
            for (QListWidgetItem *item : selectedItems()) {
                if (auto *itemWidget = dynamic_cast<ItemWidget*>(item))
                    toDelete.push_back(itemWidget);
            }

            QMessageBox::StandardButton button = QMessageBox::warning(this, "Delete Item", QString("Are you sure you want to delete %1 item(s)?").arg(toDelete.size()), QMessageBox::Yes | QMessageBox::No);
            if (button != QMessageBox::Yes)
                return;

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

void ItemListWidget::Clear() {
    // The widgets (including any that's currently playing) are about to be destroyed, so stop audio
    // first, then drop the map and the widget items together.
    StopPlayback();
    mItems.clear();
    clear();
}

void ItemListWidget::Populate(const PopulateOptions options) {
    Clear();
    mLastOptions = options;
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

    // Keep the dimmed look across refreshes for items that are part of a pending cut from this list.
    if (sClipboardIsCut && sCutSourceWidget == this) {
        for (const EmuDb::ItemSnapshot &snap : sClipboard) {
            if (snap.Type == type && snap.Id == id) {
                item->SetCut(true);
                break;
            }
        }
    }
    return true;
}

bool ItemListWidget::AddFromDatabase(EmuDb* db, ItemType type, int64_t id) {
    if (db == nullptr)
        return false;
    if (mItems.find({ type, id }) != mItems.end())
        return false;
    mItems[{ type, id }] = new ItemWidget(db, type, id, this);
    return true;
}

bool ItemListWidget::AddRemote(int64_t id, const QString& name, const QPixmap& icon,
                               const QString& originDomain, const QString& originDb) {
    if (mItems.find({ ItemType::Asset, id }) != mItems.end())
        return false;
    mItems[{ ItemType::Asset, id }] = new ItemWidget(id, name, icon, originDomain, originDb, this);
    return true;
}

bool ItemListWidget::Remove(ItemType type, int64_t id) {
    // Don't let audio outlive the widget that's playing it.
    if (mPlayingKey == std::make_pair(type, id))
        StopPlayback();

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
    // an optimization to make it less laggy
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
            QString("Export Asset %1").arg(id), startPath, "All Files (*)");
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

    QListWidgetItem *clicked = itemAt(point);
    if (clicked != nullptr) {
        // Right-clicked directly on an item: show its menu (no paste here).
        auto *itemWidget = dynamic_cast<ItemWidget*>(clicked);
        mOnContextMenuShown(&menu, itemWidget);
    } else {
        // Right-clicked a blank area: offer paste only (and only when there's something to paste).
        if (sClipboard.empty())
            return;
        QAction* paste = menu.addAction(QIcon(":/images/silk/paste_plain.png"), "Paste");
        connect(paste, &QAction::triggered, [this]() { PasteItems(); });
    }

    menu.exec(globalPos);
}

void ItemListWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->position().toPoint();
        QListWidgetItem *item = itemAt(pos);
        if (auto *itemWidget = dynamic_cast<ItemWidget*>(item)) {
            if (itemWidget->IsPlayable() && PlayBadgeRect(item).contains(pos)) {
                TogglePlayItem(itemWidget);
                // Swallow this press and the rest of the gesture so dragging off the badge doesn't
                // start a rubber-band selection.
                mBadgePressActive = true;
                event->accept();
                return;
            }
        }
    }
    QListWidget::mousePressEvent(event);
}

void ItemListWidget::mouseMoveEvent(QMouseEvent *event) {
    if (mBadgePressActive) {
        event->accept();
        return;
    }
    QListWidget::mouseMoveEvent(event);
}

void ItemListWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (mBadgePressActive && event->button() == Qt::LeftButton) {
        mBadgePressActive = false;
        event->accept();
        return;
    }
    QListWidget::mouseReleaseEvent(event);
}

QRect ItemListWidget::PlayBadgeRect(QListWidgetItem *item) {
    // Ask the style where the icon (decoration) is drawn for this item, then mirror the badge
    // geometry from ItemWidget::Asset_DrawPlayBadge (bottom-right, ~1/3 of the icon).
    QStyleOptionViewItem opt;
    initViewItemOption(&opt);
    opt.rect = visualItemRect(item);
    opt.features |= QStyleOptionViewItem::HasDecoration;
    opt.decorationSize = iconSize();
    opt.icon = item->icon();
    QRect iconRect = style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, this);

    int w = iconRect.width();
    int diameter = std::max(18, w / 3);
    int margin = std::max(2, w / 32);
    QRect badge(iconRect.right() - diameter - margin + 1,
                iconRect.bottom() - diameter - margin + 1,
                diameter, diameter);
    return badge.adjusted(-3, -3, 3, 3); // a little click tolerance
}

void ItemListWidget::TogglePlayItem(ItemWidget *item) {
    if (mLastOptions.Database == nullptr)
        return;

    if (mPlayer == nullptr) {
        mPlayer = new QMediaPlayer(this);
        mAudioOutput = new QAudioOutput(this);
        mPlayer->setAudioOutput(mAudioOutput);

        // Keep the active item's badge in sync with playback (play triangle <-> pause bars).
        connect(mPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
            auto it = mItems.find(mPlayingKey);
            if (it != mItems.end() && it->second)
                it->second->SetPlaying(state == QMediaPlayer::PlayingState);
        });
    }

    auto key = std::make_pair(item->GetType(), item->GetId());

    // Clicking the badge of the item that's already loaded toggles pause/resume, so playback picks
    // up from where it left off instead of restarting.
    if (mPlayingKey == key) {
        if (mPlayer->playbackState() == QMediaPlayer::PlayingState)
            mPlayer->pause();
        else
            mPlayer->play();
        return;
    }

    std::vector<unsigned char> data;
    if (mLastOptions.Database->RetrieveDecodedAssetData(item->GetId(), 0, &data) != SqlDb::Response::Success || data.empty())
        return;

    // Switching to a different item: revert the previously-playing item's badge to a play triangle.
    auto prev = mItems.find(mPlayingKey);
    if (prev != mItems.end() && prev->second)
        prev->second->SetPlaying(false);

    // Play from a temp file (reliable across multimedia backends). Release the old source/file first.
    mPlayer->stop();
    mPlayer->setSource(QUrl());
    delete mPlayTempFile;
    // Give the temp file the data's real extension; backends (FFmpeg/WMF) rely on it to pick the
    // right demuxer, and without it some clips report a wrong duration and cut off early.
    mPlayTempFile = new QTemporaryFile(this);
    mPlayTempFile->setFileTemplate(QDir::tempPath() + "/nwplay_XXXXXX." + DetectAssetExtension(data));
    if (!mPlayTempFile->open())
        return;
    mPlayTempFile->write(reinterpret_cast<const char*>(data.data()), static_cast<qint64>(data.size()));
    mPlayTempFile->flush();
    mPlayTempFile->close();

    mPlayingKey = key;
    mPlayer->setSource(QUrl::fromLocalFile(mPlayTempFile->fileName()));
    mPlayer->play();
}

void ItemListWidget::StopPlayback() {
    if (mPlayer != nullptr)
        mPlayer->stop();
    mPlayingKey = { ItemType::Asset, -1 };
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

    // Un-fade whatever a previous cut had dimmed before this copy/cut takes over the clipboard.
    ClearCutAppearance();

    sClipboard = std::move(snapshots);
    sClipboardIsCut = cut;
    sCutSourceDb = cut ? mLastOptions.Database : nullptr;
    sCutSourceWidget = cut ? this : nullptr;

    // Dim the freshly-cut items so it's clear they'll move on paste.
    if (cut) {
        for (const EmuDb::ItemSnapshot &snap : sClipboard) {
            if (auto *iw = GetItemWidget(snap.Type, snap.Id))
                iw->SetCut(true);
        }
    }

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
        EmuDb::ItemSnapshot toImport = snap; // a working copy we may re-id below
        bool overwrite = false;

        if (target->DoesItemExist(toImport.Type, toImport.Id)) {
            if (skipAll)
                continue;
            if (overwriteAll) {
                overwrite = true;
            } else {
                // Offer the user a real choice: overwrite, paste under a new id, or skip.
                QMessageBox box(this);
                box.setWindowTitle("Paste");
                box.setIcon(QMessageBox::Question);
                box.setText(QString("An item with id %1 already exists in this database.").arg(toImport.Id));
                box.setInformativeText("Overwrite it, paste under a new id, or skip it?");
                QPushButton* overwriteBtn = box.addButton("Overwrite", QMessageBox::AcceptRole);
                QPushButton* changeIdBtn = box.addButton("Change ID…", QMessageBox::ActionRole);
                QPushButton* skipBtn = box.addButton("Skip", QMessageBox::RejectRole);
                // "Apply to all" only makes sense for overwrite/skip (each new id must be unique).
                QCheckBox* applyAll = new QCheckBox("Apply to all remaining conflicts", &box);
                box.setCheckBox(applyAll);
                box.exec();
                QAbstractButton* answer = box.clickedButton();

                if (answer == overwriteBtn) {
                    overwrite = true;
                    if (applyAll->isChecked())
                        overwriteAll = true;
                } else if (answer == changeIdBtn) {
                    // Prompt until we get a valid, currently-free id (or the user cancels -> skip).
                    int64_t newId = toImport.Id;
                    bool gotFreeId = false;
                    while (true) {
                        bool ok = false;
                        QString text = QInputDialog::getText(this, "Change ID",
                            QString("Enter a new id for the pasted item (currently %1):").arg(toImport.Id),
                            QLineEdit::Normal, QString::number(newId), &ok);
                        if (!ok)
                            break; // cancelled

                        bool parsed = false;
                        int64_t candidate = text.trimmed().toLongLong(&parsed);
                        if (!parsed || candidate <= 0) {
                            QMessageBox::warning(this, "Change ID", "Please enter a valid positive number.");
                            continue;
                        }
                        newId = candidate;
                        if (target->DoesItemExist(toImport.Type, newId)) {
                            QMessageBox::warning(this, "Change ID", QString("Id %1 is also already taken.").arg(newId));
                            continue;
                        }
                        gotFreeId = true;
                        break;
                    }
                    if (!gotFreeId)
                        continue; // skip this item
                    target->ReassignSnapshotId(toImport, newId);
                    // overwrite stays false: the new id is free.
                } else { // Skip (or the dialog was closed)
                    (void)skipBtn;
                    if (applyAll->isChecked())
                        skipAll = true;
                    continue;
                }
            }
        }

        SqlDb::Response res = target->ImportItem(toImport, overwrite);
        if (res != SqlDb::Response::Success) {
            failures << QString::number(toImport.Id);
            continue;
        }

        // Reflect the paste in this list. For an overwrite the widget already exists, so refresh it
        // in place. Deleting and re-adding glitches the list's batched layout (the row can vanish
        // until the next repopulate). A brand-new / re-id'd item is simply added.
        if (auto *existing = GetItemWidget(toImport.Type, toImport.Id))
            existing->Reload();
        else
            Add(toImport.Type, toImport.Id);
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
        // Un-fade any source items still present (e.g. a cut+paste within the same database, where
        // the originals weren't deleted).
        ClearCutAppearance();
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

void ItemListWidget::ClearCutAppearance() {
    if (!sCutSourceWidget)
        return;
    for (const EmuDb::ItemSnapshot &snap : sClipboard) {
        if (auto *iw = sCutSourceWidget->GetItemWidget(snap.Type, snap.Id))
            iw->SetCut(false);
    }
}

void ItemListWidget::RenameItem(ItemWidget *item) {
    if (mLastOptions.Database == nullptr || item == nullptr)
        return;

    ItemType type = item->GetType();
    int64_t id = item->GetId();
    const std::string tableName = GetTableNameFromItemType(type);

    // Seed the prompt with the current name.
    QString current;
    {
        Statement stmt = mLastOptions.Database->PrepareStatement("SELECT Name FROM \"" + tableName + "\" WHERE Id = ?;");
        stmt.Bind(1, id);
        if (stmt.Step() == SQLITE_ROW)
            current = QString::fromStdString(stmt.GetStringFromColumnIndex(0));
    }

    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, current, &ok);
    if (!ok || newName == current)
        return;

    Statement stmt = mLastOptions.Database->PrepareStatement("UPDATE \"" + tableName + "\" SET Name = ? WHERE Id = ?;");
    stmt.Bind(1, newName.toStdString());
    stmt.Bind(2, id);
    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::warning(this, "Rename",
            QString("Failed to rename the item.\nLast error: %1")
                .arg(QString::fromStdString(mLastOptions.Database->GetLastErrorMsg())));
        return;
    }
    mLastOptions.Database->MarkDirty();
    item->RefreshName();
}