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
// File: BackupTask.cpp
// Started by: Hattozo
// Started on: 12/28/2025
// Description:
#include "BackupTask.h"
#include "Application.h"

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

BackupTaskItemWidget::BackupTaskItemWidget(QWidget *parent) : BackgroundTaskItemWidget(parent) {
    mTreeView = new BackupTreeView(this);
    mLayout->addWidget(mTreeView);
}

BackupTreeView* BackupTaskItemWidget::GetTreeView() {
    return mTreeView;
}

BackupTask::BackupTask(Core *core, Backup::ProcessOptions options) :
    mCore(core),
    mOptions(std::move(options)),
    mProc(nullptr)
{
}

BackupTask::~BackupTask() {

}

void BackupTask::Register(BackgroundTasks* parent) {
    BackgroundTask::Register(parent);
    SetTitle("Importing from Roblox...");
    SetCaption("Queued");
    SetProgress(0.0);
    mOptions.Callback = [this](Backup::State state, std::string msg, double progress) {
        SetCaption(QString::fromStdString(msg));
        SetProgress(progress);
    };
}

void BackupTask::OnStart() {
    mProc = gApp->GetCore()->CreateBackupProcess(mOptions);
    mProc->Start();

    if (auto *w = dynamic_cast<BackupTaskItemWidget*>(mItemWidget))
        w->GetTreeView()->SetDescriptor(mProc->GetRoot());
}

void BackupTask::OnPause() {

}

void BackupTask::OnCancel(BackgroundTaskCancelReason reason) {

}

BackgroundTaskItemWidget* BackupTask::CreateItemWidget(QWidget *parent) {
    return new BackupTaskItemWidget(parent);
}
