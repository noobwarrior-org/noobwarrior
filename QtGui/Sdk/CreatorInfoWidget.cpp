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
// File: CreatorInfoWidget.cpp
// Started by: Hattozo
// Started on: 2/8/2026
// Description:
#include "CreatorInfoWidget.h"
#include "NoobWarrior/Roblox/Api/User.h"

#include <NoobWarrior/EmuDb/ContentImages.h>

#include <QPainter>
#include <QPainterPath>

using namespace NoobWarrior;

CreatorInfoWidget::CreatorInfoWidget(QWidget* parent) : QWidget(parent),
    mMainLayout(new QHBoxLayout(this)),
    mContentLayout(new QVBoxLayout()),
    mImageLabel(new QLabel()),
    mNameLabel(new QLabel("No One!")),
    mTypeLabel(new QLabel("Type: N/A")),
    mIdLabel(new QLabel("Id: N/A"))
{
    mMainLayout->addWidget(mImageLabel);
    mMainLayout->addLayout(mContentLayout);

    mContentLayout->addWidget(mNameLabel);
    mContentLayout->addWidget(mTypeLabel);
    mContentLayout->addWidget(mIdLabel);

    mTypeLabel->setForegroundRole(QPalette::PlaceholderText);
    mIdLabel->setForegroundRole(QPalette::PlaceholderText);
    mImageLabel->setPixmap(PlaceholderAvatar());
}

QPixmap CreatorInfoWidget::PlaceholderAvatar() const {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().color(QPalette::Button));
    painter.drawEllipse(QRectF(0, 0, 64, 64));
    painter.setBrush(palette().color(QPalette::PlaceholderText));
    painter.drawEllipse(QRectF(22, 14, 20, 20));
    painter.setClipRect(QRectF(0, 0, 64, 64));
    QPainterPath body;
    body.addEllipse(QRectF(12, 38, 40, 34));
    QPainterPath circle;
    circle.addEllipse(QRectF(0, 0, 64, 64));
    painter.drawPath(body.intersected(circle));
    return pixmap;
}

void CreatorInfoWidget::Update(EmuDb* db, int64_t id, Roblox::CreatorType type) {
    if (type == Roblox::CreatorType::User || type == Roblox::CreatorType::Group)
        mTypeLabel->setText(QString("Type: %1").arg(type == Roblox::CreatorType::User ? "User" : "Group"));
    else {
        return;
    }

    Statement stmt = db->PrepareStatement(std::format("SELECT * FROM {} WHERE Id = ?;", type == Roblox::CreatorType::User ? "User" : "Group"));
    stmt.Bind(1, id);
    if (stmt.Step() == SQLITE_ROW) {
        std::map<std::string, SqlValue> columns = stmt.GetColumnMap();
        mNameLabel->setText(QString::fromStdString(std::get<std::string>(columns["Name"])));
        mIdLabel->setText(QString::number(std::get<int64_t>(columns["Id"])));
        
        std::vector<unsigned char> imageData = db->RetrieveImageData(
            type == Roblox::CreatorType::User ? ItemType::User : ItemType::Group, id);
        QImage image;
        if (image.loadFromData(imageData))
            mImageLabel->setPixmap(QPixmap::fromImage(image).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        mNameLabel->setText("No One!");
        mIdLabel->setText("Id: N/A");
        mImageLabel->setPixmap(PlaceholderAvatar());
    }
}
