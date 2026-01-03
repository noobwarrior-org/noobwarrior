// === noobWarrior ===
// File: BrowserItem.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include "../DatabaseEditor.h"

#include <NoobWarrior/Database/Database.h>
#include <QListWidgetItem>

namespace NoobWarrior {
template<typename Item>
class BrowserItem : public QListWidgetItem {
public:
    BrowserItem(const Item &item, Database *db, QListWidget *listview = nullptr)  :
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

    void Configure(DatabaseEditor *editor) {
        // auto editDialog = ContentEditorDialog<T>(editor, mRecord.Id);
        // editDialog.exec();
    }
private:
    const Item &mItem;
};
}
