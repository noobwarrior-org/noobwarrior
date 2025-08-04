// === noobWarrior ===
// File: BackgroundTask.cpp
// Started by: Hattozo
// Started on: 8/3/2025
// Description: The little progress bar that you see at the bottom right that shows all the ongoing tasks.
#include "BackgroundTask.h"

#include "NoobWarrior/Log.h"

using namespace NoobWarrior;

BackgroundTasks::BackgroundTasks(BackgroundTasksStatusBarWidget *statusBarWidget, BackgroundTasksPopupWidget *popupWidget) :
    mStatusBarWidget(statusBarWidget),
    mPopupWidget(popupWidget)
{}

BackgroundTask* BackgroundTasks::AddTask(const BackgroundTask &item) {
    mTasks.push_back(item);
    UpdateTask(&mTasks.back(), 0);
    return &mTasks.back();
}

void BackgroundTasks::UpdateTask(BackgroundTask *task, double progress, const QString &newCaption) {
    for (int i = 0; i < mTasks.size(); i++) {
        if (&mTasks[i] == task && i == 0) {
            if (mStatusBarWidget != nullptr) {
                mStatusBarWidget->mLabel.setVisible(progress < 1);
                mStatusBarWidget->mProgressBar.setVisible(progress < 1);

                if (progress < 1) {
                    mStatusBarWidget->mLabel.setText(task->Title);
                    mStatusBarWidget->mProgressBar.setValue(static_cast<int>(progress * 100));
                }
            }

            if (progress >= 1) {
                mTasks.erase(mTasks.begin() + i);
            }
            return;
        }
    }
    if (mStatusBarWidget != nullptr) {
        mStatusBarWidget->mLabel.setVisible(!mTasks.empty());
        mStatusBarWidget->mProgressBar.setVisible(!mTasks.empty());
    }
}

void BackgroundTasks::SetStatusBarWidget(BackgroundTasksStatusBarWidget *statusBarWidget) {
    mStatusBarWidget = statusBarWidget;
}

void BackgroundTasks::SetPopupWidget(BackgroundTasksPopupWidget *popupWidget) {
    mPopupWidget = popupWidget;
}

BackgroundTasksStatusBarWidget::BackgroundTasksStatusBarWidget(QWidget *parent) : QWidget(parent),
    mLayout(this)
{
    mLayout.setSizeConstraint(QLayout::SetFixedSize);
    mLayout.addWidget(&mLabel);
    mLayout.addWidget(&mProgressBar);

    mLabel.setVisible(false);
    mProgressBar.setVisible(false);
}

BackgroundTasksPopupWidget::BackgroundTasksPopupWidget(QWidget *parent) : QWidget(parent) {
}
