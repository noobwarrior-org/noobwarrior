// === noobWarrior ===
// File: BackgroundTask.h
// Started by: Hattozo
// Started on: 8/3/2025
// Description: The little progress bar that you see at the bottom right that shows all the background tasks.
#pragma once
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <vector>

namespace NoobWarrior {
struct BackgroundTask {
    QString Title   {};
    QString Caption {};
};

// Little progress bar that you see in your status bar
class BackgroundTasksStatusBarWidget : public QWidget {
    Q_OBJECT
    friend class BackgroundTasks;
public:
    BackgroundTasksStatusBarWidget(QWidget *parent = nullptr);
private:
    QHBoxLayout     mLayout;
    QLabel          mLabel;
    QProgressBar    mProgressBar;
};

// The full dialog that pops up when you click on that little progress bar
class BackgroundTasksPopupWidget : public QWidget {
    Q_OBJECT
    friend class BackgroundTasks;
public:
    BackgroundTasksPopupWidget(QWidget *parent = nullptr);
};

/**
 * @brief Manages background tasks by displaying them to the status bar that you see at the bottom.
 * It is important to note that this system does not actually do any background work on its own: you must update it
 * through callbacks in your functions.
 */
class BackgroundTasks {
public:
    BackgroundTasks(BackgroundTasksStatusBarWidget *statusBarWidget = nullptr, BackgroundTasksPopupWidget *popupWidget = nullptr);
    BackgroundTask* AddTask(const BackgroundTask &item);

    /**
     * @brief Updates a task with a new percentage, and a new caption if you want to do that.
     * @param task A task returned by AddTask()
     * @param progress A 0-1 decimal that updates the progress bar accordingly. If this is higher than 1 then it will assume the task is complete and erase it from the list.
     * @param newCaption What you want to update the caption with. If left blank, it will stay the same.
     */
    void UpdateTask(BackgroundTask *task, double progress, const QString &newCaption = "");

    void SetStatusBarWidget(BackgroundTasksStatusBarWidget *statusBarWidget);
    void SetPopupWidget(BackgroundTasksPopupWidget *popupWidget);
private:
    std::vector<BackgroundTask> mTasks;

    BackgroundTasksStatusBarWidget *mStatusBarWidget;
    BackgroundTasksPopupWidget *mPopupWidget;
};
}