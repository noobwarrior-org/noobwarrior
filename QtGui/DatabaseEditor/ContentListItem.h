// === noobWarrior ===
// File: ContentListItem.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include "DatabaseEditor.h"

#include <NoobWarrior/Database/Database.h>

#include <QListWidgetItem>

namespace NoobWarrior {
class ContentListItem : public QListWidgetItem {
public:
    ContentListItem(const Reflection::IdType &idType, int64_t id, Database *db, QListWidget *listview = nullptr);

    void Configure(DatabaseEditor *editor);
private:
    const Reflection::IdType &mIdType;
    int64_t mId;
};
}
