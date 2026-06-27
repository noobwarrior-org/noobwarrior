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

#include <thread>

namespace NoobWarrior {
class Sdk;
class Project;
class Notification;

// Drives a Backup::Process on a worker thread so the UI stays responsive, reporting progress through
// a notification toast. All progress reporting and database writes are marshalled back onto the
// Qt/event-loop thread (see Start), because the destination EmuDb connection is owned by that thread.
class BackupTask {
public:
    BackupTask(Core* core, Backup::ProcessOptions options);
    ~BackupTask();

    void SetSdk(Sdk* sdk) { mSdk = sdk; }
    void SetProject(Project* project) { mProject = project; }

    // Posts the progress notification and kicks off the worker. SetSdk must be called first.
    void Start();
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
    Notification* mNotification { nullptr };
    BackupTreeView* mTreeView { nullptr };
    bool mTreeShown { false };
};
}
