// === noobWarrior ===
// File: ContentListItem.cpp
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "ContentListItem.h"

NoobWarrior::ContentListItem::ContentListItem(Database *db, IdRecord *record, QListWidget *listview) : QListWidgetItem(listview) {
    setText(QString("%1\n(%2)").arg(QString::fromStdString(record->Name), QString::number(record->Id)));

    std::vector<unsigned char> imageData = db->RetrieveContentIconData<Asset>(record->Id);
    if (!imageData.empty()) {
        QImage image;
        image.loadFromData(imageData);

        QPixmap pixmap = QPixmap::fromImage(image);

        setIcon(QIcon(pixmap));
    }
}
