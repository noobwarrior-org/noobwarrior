// === noobWarrior ===
// File: ContentListItem.cpp
// Started by: Hattozo
// Started on: 11/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#include "ContentListItem.h"

using namespace NoobWarrior;

ContentListItem::ContentListItem(const Reflection::IdType &idType, int64_t id, Database *db, QListWidget *listview) :
    QListWidgetItem(listview),
    mIdType(idType),
    mId(id)
{
    setText(QString("%1\n(%2)").arg(QString::fromStdString(mIdType.Name), QString::number(mId)));

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

void ContentListItem::Configure(DatabaseEditor *editor) {
    // auto editDialog = ContentEditorDialog<T>(editor, mRecord.Id);
    // editDialog.exec();
}