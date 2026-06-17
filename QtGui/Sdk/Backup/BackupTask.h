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
// File: BackupTask.h
// Started by: Hattozo
// Started on: 12/28/2025
// Description:
#pragma once
#include "Sdk/BackgroundTask/BackgroundTask.h"
#include "BackupTreeView.h"
#include <NoobWarrior/Backup.h>

#include <thread>

namespace NoobWarrior {
class Sdk;
class Project;

class BackupTaskItemWidget : public BackgroundTaskItemWidget {
    Q_OBJECT
public:
    BackupTaskItemWidget(QWidget *parent = nullptr);
    BackupTreeView* GetTreeView();
private:
    BackupTreeView *mTreeView;
};

// Drives a Backup::Process on a worker thread so the UI stays responsive. All progress reporting
// and database writes are marshalled back onto the Qt/event-loop thread (see OnStart), because the
// destination EmuDb connection is owned by that thread.
class BackupTask : public BackgroundTask {
public:
    BackupTask(Core* core, Backup::ProcessOptions options);
    ~BackupTask();

    void SetSdk(Sdk* sdk) { mSdk = sdk; }
    void SetProject(Project* project) { mProject = project; }

    void Register(BackgroundTasks* parent) override;
    void OnStart() override;
    void OnPause() override;
    void OnCancel(BackgroundTaskCancelReason reason) override;

    BackgroundTaskItemWidget* CreateItemWidget(QWidget *parent = nullptr) override;
private:
    // Both run on the event-loop (UI) thread, marshalled from the worker thread.
    void OnProgress(Backup::State state, const QString& message, double progress);
    void OnWorkerFinished(Backup::Response response);

    Core* mCore;
    Backup::ProcessOptions mOptions;
    Backup::Process* mProc { nullptr };

    std::thread mWorker;
    Sdk* mSdk { nullptr };
    Project* mProject { nullptr };
    bool mTreeShown { false };
};
}