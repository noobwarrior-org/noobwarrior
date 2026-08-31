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
#include "Sdk/Project/Project.h"
#include "Sdk/Notification/Notification.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <chrono>
#include <filesystem>
#include <future>

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

// Writes per commit checkpoint; project connections run with autoCommit off.
static constexpr int kCheckpointEveryNWrites = 25;

BackupTask::BackupTask(Core *core, Backup::ProcessOptions options, Sdk *sdk) :
    QObject(sdk),
    mCore(core),
    mOptions(std::move(options)),
    mSdk(sdk)
{
}

BackupTask::~BackupTask() {
    // Queued progress/completion lambdas check this before touching us.
    *mAlive = false;

    if (mProc)
        mProc->Cancel();

    // Joining outright would deadlock: the worker may be parked on a lambda only this thread can
    // run. Drain the Core event queue until it reports done, then join.
    while (mWorker.joinable() && !mWorkerDone.load()) {
        mCore->ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (mWorker.joinable())
        mWorker.join();

    // Early teardown must not leave the run's writes in the project's open transaction.
    if (mSavepointOpen && mOptions.DestinationType == Backup::DestinationType::Database &&
        mOptions.Destination != nullptr) {
        EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);
        db->ExecStatement("ROLLBACK TO backup_run;");
        db->ExecStatement("RELEASE backup_run;");
        mSavepointOpen = false;
    }

    // Hand the tree over before the Process frees it; complete the toast so it can fade.
    if (mTreeView && mProc && mProc->GetRoot() != nullptr)
        mTreeView->GetModel()->SetRoot(mProc->ReleaseRoot(), true);
    if (mNotification && !mFinished) {
        mNotification->SetMessage("Cancelled");
        mNotification->SetProgress(1.0);
    }

    delete mProc;
}

void BackupTask::Start() {
    Core* core = mCore;
    std::shared_ptr<bool> alive = mAlive;

    mNotification = mSdk->GetNotifications()->StartTask("Importing from Roblox...", "Queued");
    mTreeView = new BackupTreeView();
    mNotification->SetContent(mTreeView);

    // Dismissing a still-running task's toast doubles as its cancel button.
    connect(mNotification, &Notification::closed, this, [this](Notification*) {
        mNotification = nullptr;
        mTreeView = nullptr;
        if (!mFinished && mProc)
            mProc->Cancel();
    });

    // Core::RunOnEventLoop() is the marshal-onto-the-UI-thread primitive.

    // Progress: hop to the UI thread, then update the notification.
    mOptions.Callback = [this, core, alive](Backup::State state, std::string msg, double progress) {
        core->RunOnEventLoop([this, alive, state, msg, progress]() {
            if (!*alive)
                return;
            OnProgress(state, QString::fromStdString(msg), progress);
        });
    };

    // The EmuDb connection belongs to the UI thread; the worker is parked on the future, so
    // done.set_value() must run unconditionally.
    mOptions.DbExecutor = [this, core, alive](const std::function<void()>& work) {
        std::promise<void> done;
        std::future<void> fut = done.get_future();
        core->RunOnEventLoop([this, alive, &work, &done]() {
            work();
            if (*alive)
                MaybeCheckpoint();
            done.set_value();
        });
        fut.wait();
    };

    if (mOptions.DestinationType == Backup::DestinationType::Database && mOptions.Destination != nullptr) {
        EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);
        // A commit would also persist the user's own uncommitted edits; leave it to them then.
        mDbCleanAtStart = !db->IsDirty();
        // All writes go inside a savepoint so a cancelled run rolls back.
        mSavepointOpen = db->ExecStatement("SAVEPOINT backup_run;");
    }

    mProc = core->CreateBackupProcess(mOptions);

    mWorker = std::thread([this, core, alive]() {
        Backup::Response response = mProc->Start();
        // done-flag before the completion hop, or the destructor's drain could wait forever.
        mWorkerDone.store(true);
        core->RunOnEventLoop([this, alive, response]() {
            if (!*alive)
                return;
            OnWorkerFinished(response);
        });
    });
}

void BackupTask::MaybeCheckpoint() {
    if (mOptions.DestinationType != Backup::DestinationType::Database || mOptions.Destination == nullptr)
        return;
    if (!mDbCleanAtStart)
        return; // committing would persist the user's own pending edits along with ours
    if (++mDbWritesSinceCheckpoint < kCheckpointEveryNWrites)
        return;
    mDbWritesSinceCheckpoint = 0;
    EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);
    // A manual Ctrl+S mid-run releases the savepoint; it is re-opened around the next batch.
    if (mSavepointOpen)
        db->ExecStatement("RELEASE backup_run;");
    db->WriteChangesToDisk();
    mSavepointOpen = db->ExecStatement("SAVEPOINT backup_run;");
}

void BackupTask::OnProgress(Backup::State state, const QString& message, double progress) {
    if (mNotification) {
        mNotification->SetMessage(message);
        // Negative = indeterminate bar. Clamp below 1.0: completing arms auto-dismiss, and
        // dismissal doubles as cancel, only OnWorkerFinished may complete the toast.
        double shown = progress;
        if (!mFinished && shown >= 1.0)
            shown = 0.99;
        mNotification->SetProgress(shown);
    }

    // Later tree mutations are marshalled onto this thread through DbExecutor, so handing the
    // live tree to the model is safe; ParsingFile reports re-root it to pick up appended children.
    if (!mTreeShown && state == Backup::State::Downloading && mTreeView) {
        mTreeShown = true;
        mTreeView->SetDescriptor(mProc->GetRoot());
    } else if (mTreeShown && state == Backup::State::ParsingFile && mTreeView && mProc && mProc->GetRoot() != nullptr) {
        mTreeView->SetDescriptor(mProc->GetRoot());
    }
}

void BackupTask::OnWorkerFinished(Backup::Response response) {
    if (mWorker.joinable()) {
        // RunOnEventLoop's failure path runs this inline on the worker; a self-join would throw.
        if (mWorker.get_id() == std::this_thread::get_id())
            mWorker.detach();
        else
            mWorker.join();
    }
    mFinished = true;

    // Make sure the tree is shown even if the backup produced no downloadable items.
    if (!mTreeShown && mProc && mTreeView) {
        mTreeShown = true;
        mTreeView->SetDescriptor(mProc->GetRoot());
    }

    const int failedDownloads = mProc ? mProc->GetFailedDownloadCount() : 0;
    const int failedWrites    = mProc ? mProc->GetFailedWriteCount() : 0;
    const int ratelimited     = mProc ? mProc->GetRatelimitedCount() : 0;
    const int failures = failedDownloads + failedWrites;

    // Commit, or the toast would claim success while nothing is durable. Left to the user when
    // the project had unsaved edits (see Start); cancelled/failed runs roll back.
    bool saved = true;
    bool commitLeftToUser = false;
    if (mOptions.DestinationType == Backup::DestinationType::Database && mOptions.Destination != nullptr) {
        EmuDb* db = static_cast<EmuDb*>(mOptions.Destination);
        if (response == Backup::Response::Ok) {
            if (mSavepointOpen) {
                db->ExecStatement("RELEASE backup_run;");
                mSavepointOpen = false;
            }
            if (mDbCleanAtStart) {
                saved = mProject != nullptr ? mProject->Save()
                                            : db->WriteChangesToDisk() == SqlDb::Response::Success;
                if (!saved && !db->IsDirty())
                    saved = true; // nothing left to commit (e.g. a checkpoint already flushed everything)
            } else {
                commitLeftToUser = true;
            }
        } else if (mSavepointOpen) {
            // This also rolls back edits the user made in this project DURING the run: SQLite
            // savepoints nest strictly, so backup residue and mid-run edits cannot be separated.
            // Editing a database while it is being backed up into is the rarer case.
            db->ExecStatement("ROLLBACK TO backup_run;");
            db->ExecStatement("RELEASE backup_run;");
            mSavepointOpen = false;
        }
    }

    if (mNotification) {
        if (response == Backup::Response::Ok) {
            QString msg = failures > 0 ? QString("Done! There are %1 errors.").arg(failures)
                                       : QString("Done!");
            if (commitLeftToUser)
                msg += " Also, the database is not not saved yet: the project has other unsaved changes, use Save Project";
            else if (!saved)
                msg += " Also, saving the database failed, use Save Project to retry";
            mNotification->SetMessage(msg);
        } else if (response == Backup::Response::Cancelled) {
            mNotification->SetMessage("Cancelled");
        } else if (response == Backup::Response::DestinationInvalid) {
            mNotification->SetMessage("Failed: the backup destination is invalid");
        } else if (response == Backup::Response::SourceInvalid) {
            mNotification->SetMessage("Failed: the source file could not be read as a Roblox place or model");
        }
        mNotification->SetProgress(1.0); // mark complete so the toast can dismiss itself
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
        // This action outlives the task and possibly the project (the notification history keeps
        // it clickable for the whole session), so it must not capture the Project or its EmuDb,
        // only values. The database itself was already committed above.
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
                "Assets the engine requests will now be saved into this database. Launch the game to start grabbing. Don't forget to turn Asset Grab Mode off once you're done!");
        } });

        QString title = failures > 0 ? QString("Backup finished with %1 errors").arg(failures)
                                     : QString("Backup complete");
        QString body;
        if (failures > 0) {
            body = QString("%1 downloads or writes failed").arg(failures);
            if (ratelimited > 0)
                body += QString(" (Roblox rate-limited %1 requests - retrying later may recover them)").arg(ratelimited);
            body += ". ";
        } else if (ratelimited > 0) {
            body = QString("Roblox rate-limited %1 requests, so some names or thumbnails may be "
                           "missing - re-running the backup later can fill them in. ").arg(ratelimited);
        }
        body += "Some assets only surface while a game actually runs. Turn on Asset Grab Mode and "
                "play the game; every asset the engine downloads will be saved straight into this database.";

        mSdk->GetNotifications()->Notify(title, body, actions);
    }

    // The toast (and its tree view) can outlive this task, so hand the descriptor tree over to the
    // model before the Process that owned it is destroyed.
    if (mTreeView && mProc)
        mTreeView->GetModel()->SetRoot(mProc->ReleaseRoot(), true);

    deleteLater();
}
