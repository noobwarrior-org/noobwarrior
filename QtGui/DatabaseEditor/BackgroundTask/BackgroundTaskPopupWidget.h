// === noobWarrior ===
// File: BackgroundTaskPopupWidget.h
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The full dialog that pops up when you click on that little progress bar
#pragma once
#include <QDockWidget>
#include <QScrollArea>
#include <QWidget>

namespace NoobWarrior {
class BackgroundTaskPopupWidget : public QDockWidget {
    Q_OBJECT
    friend class BackgroundTasks;
public:
    BackgroundTaskPopupWidget(QWidget *parent = nullptr);
    void InitWidgets();
private:
    QScrollArea* mScrollArea;
    QWidget* mWidget;
};
}