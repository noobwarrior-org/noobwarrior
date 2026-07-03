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
// File: ItemWidget.cpp
// Started by: Hattozo
// Started on: 2/14/2026
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "ItemWidget.h"
#include "NoobWarrior/EmuDb/ItemType.h"
#include "Sdk/Sdk.h"
#include "Sdk/Item/ItemDialog.h"

#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QBrush>

#include <algorithm>
#include <cassert>

using namespace NoobWarrior;

ItemWidget::ItemWidget(EmuDb *db, NoobWarrior::ItemType type, int64_t id, QListWidget *listview) :
    QListWidgetItem(listview),
    mDb(db),
    mType(type),
    mId(id)
{
    assert(db != nullptr && "ItemWidget: Passed database is null");
    Reload();
}

ItemWidget::ItemWidget(int64_t id, const QString& name, const QPixmap& icon, QListWidget *listview) :
    QListWidgetItem(listview),
    mDb(nullptr),
    mType(NoobWarrior::ItemType::Asset),
    mId(id)
{
    setText(QString("%1\n(%2)").arg(name, QString::number(id)));
    if (!icon.isNull()) {
        mIconPixmap = icon;
        ApplyAppearance();
    }
}

void ItemWidget::Reload() {
    if (mDb == nullptr)
        return;
    std::string tableName = GetTableNameFromItemType(mType);
    std::string name;

    Statement stmt = mDb->PrepareStatement(std::format("SELECT Name FROM \"{}\" WHERE Id = ?;", tableName));
    if (stmt.Fail()) {
        Out("ItemWidget", "Failed to retrieve name for ID {}", mId);
        return;
    }
    stmt.Bind(1, mId);
    if (stmt.Step() == SQLITE_ROW) {
        name = stmt.GetStringFromColumnIndex(0);
    }

    setText(QString("%1\n(%2)").arg(QString::fromStdString(name), QString::number(mId)));

    // Playable assets (Audio) get a little play badge in the bottom-right corner of their icon.
    Roblox::AssetType assetType = Roblox::AssetType::None;
    if (mType == NoobWarrior::ItemType::Asset) {
        Statement typeStmt = mDb->PrepareStatement("SELECT Type FROM Asset WHERE Id = ?;");
        typeStmt.Bind(1, mId);
        if (typeStmt.Step() == SQLITE_ROW)
            assetType = static_cast<Roblox::AssetType>(typeStmt.GetIntFromColumnIndex(0));
    }
    mPlayable = assetType == Roblox::AssetType::Audio;

    QPixmap pixmap;
    std::vector<unsigned char> imageData = mDb->RetrieveImageData(mType, mId);
    if (!imageData.empty()) {
        QImage image;
        image.loadFromData(imageData);
        pixmap = QPixmap::fromImage(image);
    }

    if (mPlayable) {
        // Audio assets usually have no thumbnail; give the badge a transparent canvas to sit on.
        if (pixmap.isNull()) {
            pixmap = QPixmap(64, 64);
            pixmap.fill(Qt::transparent);
        }
        mBasePixmap = pixmap; // cache the un-badged icon so the badge can be redrawn on play/pause
        Asset_DrawMediaBadge(pixmap, false);
    } else {
        mBasePixmap = QPixmap();
    }

    mIconPixmap = pixmap;
    ApplyAppearance();
}

// Fades a pixmap to ~40% opacity (used to dim items pending a cut).
static QPixmap FadePixmap(const QPixmap &src) {
    if (src.isNull())
        return src;
    QPixmap faded(src.size());
    faded.fill(Qt::transparent);
    QPainter painter(&faded);
    painter.setOpacity(0.4);
    painter.drawPixmap(0, 0, src);
    painter.end();
    return faded;
}

void ItemWidget::ApplyAppearance() {
    setForeground(mCut ? QBrush(Qt::gray) : QBrush());
    if (!mIconPixmap.isNull())
        setIcon(QIcon(mCut ? FadePixmap(mIconPixmap) : mIconPixmap));
}

void ItemWidget::SetPlaying(bool playing) {
    if (!mPlayable || mBasePixmap.isNull())
        return;
    QPixmap pixmap = mBasePixmap;
    Asset_DrawMediaBadge(pixmap, playing);
    mIconPixmap = pixmap;
    ApplyAppearance();
}

void ItemWidget::SetCut(bool cut) {
    mCut = cut;
    ApplyAppearance();
}

void ItemWidget::RefreshName() {
    if (mDb == nullptr)
        return;
    std::string tableName = GetTableNameFromItemType(mType);
    Statement stmt = mDb->PrepareStatement(std::format("SELECT Name FROM \"{}\" WHERE Id = ?;", tableName));
    if (stmt.Fail())
        return;
    stmt.Bind(1, mId);
    std::string name;
    if (stmt.Step() == SQLITE_ROW)
        name = stmt.GetStringFromColumnIndex(0);
    setText(QString("%1\n(%2)").arg(QString::fromStdString(name), QString::number(mId)));
}

void ItemWidget::Asset_DrawMediaBadge(QPixmap &pixmap, bool playing) {
    // A dark translucent circle with a white glyph, drawn at ~1/3 the icon size in the bottom-right.
    // Painted with QPainter (rather than scaling a tiny bitmap) so it stays crisp. The glyph is a
    // pause symbol while playing, and a play triangle otherwise.
    int diameter = std::max(18, pixmap.width() / 3);
    int margin = std::max(2, pixmap.width() / 32);
    int x = pixmap.width() - diameter - margin;
    int y = pixmap.height() - diameter - margin;

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawEllipse(x, y, diameter, diameter);

    double cx = x + diameter / 2.0;
    double cy = y + diameter / 2.0;
    painter.setBrush(Qt::white);

    if (playing) {
        double barWidth = diameter * 0.12;
        double barHeight = diameter * 0.38;
        double gap = diameter * 0.10;
        painter.drawRect(QRectF(cx - gap - barWidth, cy - barHeight / 2, barWidth, barHeight));
        painter.drawRect(QRectF(cx + gap, cy - barHeight / 2, barWidth, barHeight));
    } else {
        double r = diameter * 0.26;
        QPolygonF triangle;
        triangle << QPointF(cx - r * 0.7, cy - r)
                 << QPointF(cx - r * 0.7, cy + r)
                 << QPointF(cx + r, cy);
        painter.drawPolygon(triangle);
    }
    painter.end();
}

void ItemWidget::Configure() {
    if (mDb == nullptr) // no database to open an item dialog against
        return;
    QWidget* sdk = this->listWidget();
    while (sdk && !dynamic_cast<Sdk*>(sdk)) {
        sdk = sdk->parentWidget();
    }

    ItemDialog dialog(mDb, mType, mId, sdk);
    dialog.exec();
}

NoobWarrior::ItemType ItemWidget::GetType() {
    return mType;
}

int64_t ItemWidget::GetId() {
    return mId;
}