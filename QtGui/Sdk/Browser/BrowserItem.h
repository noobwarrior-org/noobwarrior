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
// File: BrowserItem.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include "Sdk/Sdk.h"

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <QListWidgetItem>

namespace NoobWarrior {
template<typename Item>
class BrowserItem : public QListWidgetItem {
public:
    BrowserItem(const Item &item, EmuDb *db, QListWidget *listview = nullptr)  :
        QListWidgetItem(listview),
        mItem(item)
    {
        setText(QString("%1\n(%2)").arg(QString::fromStdString(mItem.Name), QString::number(mItem.Id)));

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
private:
    const Item &mItem;
};
}
