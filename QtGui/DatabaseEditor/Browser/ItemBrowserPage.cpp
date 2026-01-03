// === noobWarrior ===
// File: ItemBrowserPage.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "ItemBrowserPage.h"

using namespace NoobWarrior;

ItemBrowserPage::ItemBrowserPage(QWidget *parent) : QListWidget(parent) {
    InitWidgets();
}

void ItemBrowserPage::InitWidgets() {
    setAutoFillBackground(true);
    setMovement(QListView::Static);
    setViewMode(QListView::IconMode);
    setIconSize(QSize(64, 64));
    setWordWrap(true);

    connect(this, &QListWidget::itemDoubleClicked, this, [](QListWidgetItem *item) {
        // auto *contentItem = dynamic_cast<BrowserItem*>(item);
        // contentItem->Configure(editor);
    });
}