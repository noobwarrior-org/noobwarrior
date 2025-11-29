// === noobWarrior ===
// File: ContentListItem.h
// Started by: Hattozo
// Started on: 7/26/2025
// Description: An item for a QListWidget representing a Roblox ID, showing its name, id, and icon.
#pragma once
#include "DatabaseEditor.h"

#include <NoobWarrior/Database/Database.h>
#include <QListWidgetItem>
#include <entt/entt.hpp>

namespace NoobWarrior {
class ContentListItem : public QListWidgetItem {
public:
    ContentListItem(const entt::meta_type &itemType, int64_t id, Database *db, QListWidget *listview = nullptr);

    void Configure(DatabaseEditor *editor);
private:
    const entt::meta_type &mItemType;
    int64_t mId;
};
}
