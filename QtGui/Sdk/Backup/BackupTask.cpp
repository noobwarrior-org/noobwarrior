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
#include "Sdk/Sdk.h"
#include "Sdk/Notification/Notification.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <filesystem>
#include <future>

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
    mOptions(std::move(options))
{
}

BackupTask::~BackupTask() {
    // The task is normally kept alive for the lifetime of the SDK window (the tree view keeps a
    // pointer into mProc's descriptor tree). If we are torn down anyway, stop the worker and wait
    // for it before freeing the process it is using.
    if (mProc)
        mProc->Cancel();
    if (mWorker.joinable())
        mWorker.join();
    delete mProc;
}

void BackupTask::Register(BackgroundTasks* parent) {
    BackgroundTask::Register(parent);
    SetTitle("Importing from Roblox...");
    SetCaption("Queued");
    SetProgress(0.0);
}

void BackupTask::OnStart() {
    Core* core = mCore;

    // The Qt app pumps Core::ProcessEvents() from the event-loop thread, so Core::RunOnEventLoop()
    // is our marshal-onto-the-UI-thread primitive.

    // Progress: hop to the UI thread, then update the widgets.
    mOptions.Callback = [this, core](Backup::State state, std::string msg, double progress) {
        core->RunOnEventLoop([this, state, msg, progress]() {
            OnProgress(state, QString::fromStdString(msg), progress);
        });
    };

    // Database writes: the EmuDb connection belongs to the UI thread, so run the work there and
    // block the worker until it completes (a synchronous hand-off, as DbExecutor requires).
    mOptions.DbExecutor = [core](const std::function<void()>& work) {
        std::promise<void> done;
        std::future<void> fut = done.get_future();
        core->RunOnEventLoop([&work, &done]() {
            work();
            done.set_value();
        });
        fut.wait();
    };

    mProc = core->CreateBackupProcess(mOptions);

    mWorker = std::thread([this, core]() {
        Backup::Response response = mProc->Start();
        // Hand completion back to the UI thread; the worker has returned by the time this runs.
        core->RunOnEventLoop([this, response]() {
            OnWorkerFinished(response);
        });
    });
}

void BackupTask::OnProgress(Backup::State state, const QString& message, double progress) {
    SetCaption(message);
    if (progress >= 0.0)
        SetProgress(progress);

    // Populate is finished by the time the first download is reported, so the tree is now stable
    // and safe to hand to the model.
    if (!mTreeShown && state == Backup::State::Downloading) {
        mTreeShown = true;
        if (auto *w = dynamic_cast<BackupTaskItemWidget*>(mItemWidget))
            w->GetTreeView()->SetDescriptor(mProc->GetRoot());
    }
}

void BackupTask::OnWorkerFinished(Backup::Response response) {
    if (mWorker.joinable())
        mWorker.join();

    // Make sure the tree is shown even if the backup produced no downloadable items.
    if (!mTreeShown && mProc) {
        mTreeShown = true;
        if (auto *w = dynamic_cast<BackupTaskItemWidget*>(mItemWidget))
            w->GetTreeView()->SetDescriptor(mProc->GetRoot());
    }

    if (response == Backup::Response::Ok) {
        SetCaption("Done");
        SetProgress(1.0);
    } else if (response == Backup::Response::Cancelled) {
        SetCaption("Cancelled");
    }
    
    if (mSdk)
        mSdk->Refresh();

    if (mSdk && response == Backup::Response::Ok) {
        Core* core = mCore;
        Sdk* sdk = mSdk;

        std::string grabDbPath;
        if (mOptions.DestinationType == Backup::DestinationType::Database && mOptions.Destination != nullptr)
            grabDbPath = static_cast<EmuDb*>(mOptions.Destination)->GetFilePath().string();

        std::vector<NotificationAction> actions;
        actions.push_back({ "Enable Asset Grab Mode", [core, sdk, grabDbPath]() {
            Registry* reg = core->GetRegistry();
            reg->SetKeyValue<bool>("emu.asset_grab_mode", true);
            reg->SetKeyValue<bool>("emu.enable_roblox_proxy", true);
            if (!grabDbPath.empty())
                reg->SetKeyValue<std::string>("emu.asset_grab_db", grabDbPath);
            
            EmuDbManager* dbManager = core->GetEmuDbManager();
            if (!grabDbPath.empty() && dbManager->GetDbFromFilePath(grabDbPath) == nullptr)
                dbManager->Mount(std::filesystem::path(grabDbPath), dbManager->GetMountedDatabases().size());

            sdk->GetNotifications()->Notify("Asset Grab Mode enabled",
                "Assets the engine requests will now be saved into this database. Launch the game to start grabbing.");
        } });

        mSdk->GetNotifications()->Notify("Backup complete",
            "Want the assets from the place file in this database too? Turn on Asset Grab Mode and play the game; every asset the engine downloads will be saved straight into this database.",
            actions);
    }
}

void BackupTask::OnPause() {

}

void BackupTask::OnCancel(BackgroundTaskCancelReason reason) {
    // Just signal cancellation. The worker notices, unwinds, and reports completion via
    // OnWorkerFinished (which joins it). We must not join here: the worker may be blocked waiting
    // for this same UI thread to run a marshalled DB write.
    if (mProc)
        mProc->Cancel();
}

BackgroundTaskItemWidget* BackupTask::CreateItemWidget(QWidget *parent) {
    return new BackupTaskItemWidget(parent);
}
