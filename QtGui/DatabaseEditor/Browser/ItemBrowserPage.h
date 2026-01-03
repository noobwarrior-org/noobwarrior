// === noobWarrior ===
// File: ItemBrowserPage.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include <QListWidget>

namespace NoobWarrior {
class ItemBrowserPage : public QListWidget {
public:
    ItemBrowserPage(QWidget *parent = nullptr);
    virtual void Refresh() = 0;
    void InitWidgets();
};
}