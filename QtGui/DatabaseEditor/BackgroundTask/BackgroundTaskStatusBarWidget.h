// === noobWarrior ===
// File: BackgroundTaskStatusBarWidget.h
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The little progress bar that you see in your status bar
#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

namespace NoobWarrior {
class BackgroundTaskPopupWidget;
class BackgroundTaskStatusBarWidget : public QWidget {
    Q_OBJECT
    friend class BackgroundTasks;
public:
    BackgroundTaskStatusBarWidget(QWidget *parent = nullptr);
protected:
    void SetPopupWidget(BackgroundTaskPopupWidget *widget);
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    BackgroundTaskPopupWidget *mPopupWidget;
    
    QHBoxLayout     mLayout;
    QLabel          mLabel;
    QProgressBar    mProgressBar;
};
}