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

namespace NoobWarrior {
class BackupTaskItemWidget : public BackgroundTaskItemWidget {
    Q_OBJECT
public:
    BackupTaskItemWidget(QWidget *parent = nullptr);
    BackupTreeView* GetTreeView();
private:
    BackupTreeView *mTreeView;
};

class BackupTask : public BackgroundTask {
public:
    BackupTask(Core* core, Backup::ProcessOptions options);
    ~BackupTask();

    void Register(BackgroundTasks* parent) override;
    void OnStart() override;
    void OnPause() override;
    void OnCancel(BackgroundTaskCancelReason reason) override;

    BackgroundTaskItemWidget* CreateItemWidget(QWidget *parent = nullptr) override;
private:
    Core* mCore;
    Backup::ProcessOptions mOptions;
    Backup::Process* mProc;
};
}