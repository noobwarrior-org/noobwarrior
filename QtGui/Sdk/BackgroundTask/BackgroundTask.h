/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: BackgroundTask.h
// Started by: Hattozo
// Started on: 8/3/2025
// Description: A background task is a process that the Database Editor is running.
// Its main purpose is to display to the user that a process is ongoing, and exactly what it is doing.
#pragma once
#include <QString>
#include <vector>

namespace NoobWarrior {
enum class BackgroundTaskCancelReason {
    UserInitiatedAction,
    Failed
};

class BackgroundTask;
class BackgroundTaskStatusBarWidget;
class BackgroundTaskPopupWidget;

class BackgroundTasks {
public:
    BackgroundTasks(BackgroundTaskStatusBarWidget *statusBarWidget = nullptr, BackgroundTaskPopupWidget *popupWidget = nullptr);
    void AddTask(BackgroundTask* task);
    void RemoveTask(BackgroundTask* task);

    /**
     * @brief Updates a task with a new percentage, and a new caption if you want to do that.
     * @param task A pointer to a BackgroundTask
     * @param progress A 0-1 decimal that updates the progress bar accordingly.
     * @param newTitle What you want to update the title with. If left blank, it will stay the same.
     * @param newCaption What you want to update the caption with. If left blank, it will stay the same.
     */
    void UpdateTask(BackgroundTask* task, double progress, const QString &newTitle = "", const QString &newCaption = "");

    void SetStatusBarWidget(BackgroundTaskStatusBarWidget *statusBarWidget);
    void SetPopupWidget(BackgroundTaskPopupWidget *popupWidget);
private:
    std::vector<BackgroundTask*> mTasks;

    BackgroundTaskStatusBarWidget *mStatusBarWidget;
    BackgroundTaskPopupWidget *mPopupWidget;
};

class BackgroundTask {
public:
    BackgroundTask(BackgroundTasks* parent);

    void Start();
    void Pause();
    void Cancel(BackgroundTaskCancelReason reason = BackgroundTaskCancelReason::Failed);

    void SetTitle(const QString &title);
    void SetCaption(const QString &caption);
    void SetProgress(double progress);

    virtual QString GetTitle();
    virtual QString GetCaption();
    virtual double GetProgress();
protected:
    virtual void OnStart() = 0;
    virtual void OnPause() = 0;
    virtual void OnCancel(BackgroundTaskCancelReason reason) = 0;
private:
    BackgroundTasks* mParent;
    QString mTitle;
    QString mCaption;
    double mProgress;
};
}
