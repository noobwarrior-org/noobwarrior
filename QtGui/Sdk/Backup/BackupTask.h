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
#include "BackupTreeView.h"
#include <NoobWarrior/Backup.h>

#include <QObject>

#include <atomic>
#include <memory>
#include <thread>

namespace NoobWarrior {
class Sdk;
class Project;
class Notification;

// This handles a Backup::Process on a separate thread.
class BackupTask : public QObject {
    Q_OBJECT
public:
    BackupTask(Core* core, Backup::ProcessOptions options, Sdk* sdk);
    ~BackupTask() override;

    void SetProject(Project* project) { mProject = project; }
    Project* GetProject() const { return mProject; }

    // Posts the progress notification and kicks off the worker.
    void Start();
private:
    // Both run on the event-loop (UI) thread, marshalled from the worker thread.
    void OnProgress(Backup::State state, const QString& message, double progress);
    void OnWorkerFinished(Backup::Response response);

    // Commits every few writes so a crash mid-backup doesn't lose everything. UI thread only.
    void MaybeCheckpoint();

    Core* mCore;
    Backup::ProcessOptions mOptions;
    Backup::Process* mProc { nullptr };

    // Queued cross-thread lambdas check this before touching the task. UI thread only.
    std::shared_ptr<bool> mAlive { std::make_shared<bool>(true) };

    std::thread mWorker;
    std::atomic<bool> mWorkerDone { false }; // set by the worker right before it exits
    bool mFinished { false };                // OnWorkerFinished ran (UI thread)
    int mDbWritesSinceCheckpoint { 0 };

    // Checkpoints/auto-save only run when the project was clean at start. A commit would persist
    // the user's own pending edits. All writes sit in a "backup_run" SAVEPOINT for rollback.
    bool mDbCleanAtStart { false };
    bool mSavepointOpen { false };

    Sdk* mSdk { nullptr };
    Project* mProject { nullptr };
    Notification* mNotification { nullptr };
    BackupTreeView* mTreeView { nullptr };
    bool mTreeShown { false };
};
}
