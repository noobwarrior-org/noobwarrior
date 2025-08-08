// === noobWarrior ===
// File: ContentListItem.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Database/Record/IdRecord.h>

#include "ContentListItemBase.h"
#include "ContentEditorDialog.h"

namespace NoobWarrior {
template<class T>
class ContentListItem : public ContentListItemBase {
public:
    ContentListItem(T record, Database *db, QListWidget *listview = nullptr) : ContentListItemBase(listview),
        mRecord(std::move(record))
    {
        setText(QString("%1\n(%2)").arg(QString::fromStdString(mRecord.Name), QString::number(mRecord.Id)));

        std::vector<unsigned char> imageData = db->RetrieveContentImageData<T>(mRecord.Id);
        if (!imageData.empty()) {
            QImage image;
            image.loadFromData(imageData);

            QPixmap pixmap = QPixmap::fromImage(image);

            setIcon(QIcon(pixmap));
        }
    }

    void Configure(DatabaseEditor *editor) override {
        auto editDialog = ContentEditorDialog<T>(editor, mRecord.Id);
        editDialog.exec();
    }
private:
    T mRecord;
};
}
