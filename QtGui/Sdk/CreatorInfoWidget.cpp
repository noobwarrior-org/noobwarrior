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

    std::vector<unsigned char> data;
    data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    QImage image;
    image.loadFromData(data);

    QPixmap pixmap = QPixmap::fromImage(image);
    mImageLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    } else {
        mNameLabel->setText("No One!");
        mIdLabel->setText("Id: N/A");
    }
}
