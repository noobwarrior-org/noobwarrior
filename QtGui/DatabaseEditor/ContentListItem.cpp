// === noobWarrior ===
// File: ContentListItem.cpp
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "ContentListItem.h"

NoobWarrior::ContentListItem::ContentListItem(Database *db, IdRecord *record, QListWidget *listview) : QListWidgetItem(listview) {
    setText(QString::fromStdString(record->Name));

    std::vector<unsigned char> imageData = db->RetrieveContentIconData<Asset>(record->Id);
    if (!imageData.empty()) {
        Out("ContentListItem", "{}", imageData.size());
        QImage image;
        image.loadFromData(imageData);

        QPixmap pixmap = QPixmap::fromImage(image);

        setIcon(QIcon(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
}
