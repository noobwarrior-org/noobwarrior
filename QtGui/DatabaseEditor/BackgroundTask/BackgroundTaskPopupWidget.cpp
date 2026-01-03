// === noobWarrior ===
// File: BackgroundTaskPopupWidget.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The full dialog that pops up when you click on that little progress bar
#include "BackgroundTaskPopupWidget.h"

using namespace NoobWarrior;

BackgroundTaskPopupWidget::BackgroundTaskPopupWidget(QWidget *parent) : QDockWidget(parent) {
    InitWidgets();
}

void BackgroundTaskPopupWidget::InitWidgets() {
    mScrollArea = new QScrollArea(this);
    mWidget = new QWidget(mScrollArea);

    
}
