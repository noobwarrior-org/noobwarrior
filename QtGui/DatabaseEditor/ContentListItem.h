// === noobWarrior ===
// File: ContentListItem.cpp
// Started by: Hattozo
// Started on: 7/26/2025
// Description:
#pragma once
#include <NoobWarrior/Database/Database.h>
#include <../../Core/Include/NoobWarrior/Database/Record/IdRecord.h>
#include <QListWidgetItem>

namespace NoobWarrior {
class ContentListItem : public QListWidgetItem {
public:
    ContentListItem(Database *db, IdRecord *record, QListWidget *listview = nullptr);
};
}
