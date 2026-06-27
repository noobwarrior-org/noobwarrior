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
#include "ExampleTask.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <filesystem>
#include <future>

using namespace NoobWarrior;

ExampleTask::ExampleTask() {
}

ExampleTask::~ExampleTask() {
}

void ExampleTask::Register(BackgroundTasks* parent) {
    BackgroundTask::Register(parent);
    SetTitle("Example Task");
    SetCaption("Yeha what's up");
    SetProgress(0.0);
}

void ExampleTask::OnStart() {

}

void ExampleTask::OnPause() {

}

void ExampleTask::OnCancel(BackgroundTaskCancelReason reason) {

}

