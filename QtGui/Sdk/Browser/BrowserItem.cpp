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
// File: BrowserItem.cpp
// Started by: Hattozo
// Started on: 2/14/2026
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "BrowserItem.h"
#include "Sdk/Sdk.h"

#include <NoobWarrior/EmuDb/EmuDb.h>

using namespace NoobWarrior;

BrowserItem::BrowserItem(const int id, const std::string &name, EmuDb *db, QListWidget *listview)  :
    QListWidgetItem(listview)
{
    setText(QString("%1\n(%2)").arg(QString::fromStdString(name), QString::number(id)));

    /*
    std::vector<unsigned char> imageData = db->RetrieveContentImageData<T>(mRecord.Id);
    if (!imageData.empty()) {
        QImage image;
        image.loadFromData(imageData);

        QPixmap pixmap = QPixmap::fromImage(image);

        setIcon(QIcon(pixmap));
    }
    */
}

void Configure(Sdk *editor) {
    // auto editDialog = ContentEditorDialog<T>(editor, mRecord.Id);
    // editDialog.exec();
}