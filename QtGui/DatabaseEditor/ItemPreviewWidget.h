// === noobWarrior ===
// File: ItemPreviewWidget.h
// Started by: Hattozo
// Started on: 8/3/2025
// Description: Qt widget that lets the user view an item, as if they were viewing it on the actual Roblox website itself.
#pragma once
#include <QWidget>

namespace NoobWarrior {
template<class T>
class ItemPreviewWidget : public QWidget {
public:
    ItemPreviewWidget(const int64_t &id) :
        mId(id)
    {

    }
private:
    int64_t mId;
};
}