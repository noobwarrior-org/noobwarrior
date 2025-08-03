// === noobWarrior ===
// File: ContentListItemBase.h
// Started by: Hattozo
// Started on: 8/2/2025
// Description: The one true base for the ContentListItem template class, so that I can type cast to it
#pragma once
#include <QListWidgetItem>

namespace NoobWarrior {
    class ContentListItemBase : public QListWidgetItem {
    public:
        ContentListItemBase(QListWidget *listview = nullptr) : QListWidgetItem(listview) {}
        virtual void Configure(DatabaseEditor *editor) = 0;
    };
}
