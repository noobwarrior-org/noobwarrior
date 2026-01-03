// === noobWarrior ===
// File: BackgroundTask.cpp
// Started by: Hattozo
// Started on: 8/3/2025
// Description: The little progress bar that you see at the bottom right that shows all the ongoing tasks.
#include "BackgroundTask.h"
#include "BackgroundTaskStatusBarWidget.h"
#include "BackgroundTaskPopupWidget.h"

using namespace NoobWarrior;

BackgroundTask::BackgroundTask(BackgroundTasks* parent) : mParent(parent), mProgress(-1) {
    mParent->AddTask(this);
}

void BackgroundTask::Start() {

}

void BackgroundTask::Pause() {

}

void BackgroundTask::Cancel(BackgroundTaskCancelReason reason) {

}

void BackgroundTask::SetTitle(const QString &title) {
    mTitle = title;
    mParent->UpdateTask(this, mProgress, mTitle, mCaption);
}

void BackgroundTask::SetCaption(const QString &caption) {
    mCaption = caption;
    mParent->UpdateTask(this, mProgress, mTitle, mCaption);
}

void BackgroundTask::SetProgress(double progress) {
    mProgress = progress;
    mParent->UpdateTask(this, mProgress, mTitle, mCaption);
}

QString BackgroundTask::GetTitle() {
    return mTitle;
}

QString BackgroundTask::GetCaption() {
    return mCaption;
}

double BackgroundTask::GetProgress() {
    return mProgress;
}

BackgroundTasks::BackgroundTasks(BackgroundTaskStatusBarWidget *statusBarWidget, BackgroundTaskPopupWidget *popupWidget) :
    mStatusBarWidget(statusBarWidget),
    mPopupWidget(popupWidget)
{}

void BackgroundTasks::AddTask(BackgroundTask* task) {
    if (std::find(mTasks.begin(), mTasks.end(), task) != mTasks.end())
        return;
    mTasks.push_back(task);
    UpdateTask(mTasks.back(), task->GetProgress(), task->GetTitle(), task->GetCaption());
}

void BackgroundTasks::RemoveTask(BackgroundTask* task) {
    auto it = std::find(mTasks.begin(), mTasks.end(), task);
    if (it != mTasks.end())
        mTasks.erase(it);
}

void BackgroundTasks::UpdateTask(BackgroundTask *task, double progress, const QString &newTitle, const QString &newCaption) {
    for (int i = 0; i < mTasks.size(); i++) {
        if (mTasks[i] == task && i == 0) {
            if (mStatusBarWidget != nullptr) {
                mStatusBarWidget->mLabel.setVisible(progress < 1);
                mStatusBarWidget->mProgressBar.setVisible(progress < 1);

                if (progress < 1) {
                    if (!newTitle.isEmpty())
                        mStatusBarWidget->mLabel.setText(newTitle);
                    mStatusBarWidget->mProgressBar.setValue(static_cast<int>(progress * 100));
                }
            }
            return;
        }
    }
    if (mStatusBarWidget != nullptr) {
        mStatusBarWidget->mLabel.setVisible(!mTasks.empty());
        mStatusBarWidget->mProgressBar.setVisible(!mTasks.empty());
    }
}

void BackgroundTasks::SetStatusBarWidget(BackgroundTaskStatusBarWidget *statusBarWidget) {
    mStatusBarWidget = statusBarWidget;
}

void BackgroundTasks::SetPopupWidget(BackgroundTaskPopupWidget *popupWidget) {
    mPopupWidget = popupWidget;
    if (mStatusBarWidget != nullptr)
        mStatusBarWidget->SetPopupWidget(popupWidget);
}
