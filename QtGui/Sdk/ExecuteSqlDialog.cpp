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
// File: ExecuteSqlDialog.cpp
// Started by: Hattozo
// Started on: 2/13/2026
// Description: lets you execute sql code into the emulator database
#include "ExecuteSqlDialog.h"
#include "Sdk.h"
#include "Project/EmuDb/EmuDbProject.h"

#include <cassert>

using namespace NoobWarrior;

ExecuteSqlDialog::ExecuteSqlDialog(QWidget *parent) : QDialog(parent),
    mSdk(nullptr)
{
    setWindowTitle("Execute SQL");

    auto *sdk = dynamic_cast<Sdk*>(this->parent());
    assert(sdk != nullptr && "ExecuteSqlDialog must be parented to Sdk");
    mSdk = sdk;

    Refresh();
}

void ExecuteSqlDialog::Refresh() {
    Project* proj = mSdk->GetFocusedProject();
    auto *emuDbProj = dynamic_cast<EmuDbProject*>(proj);
    assert(emuDbProj != nullptr && "EmuDbProject must be the currently focused project.");
    if (emuDbProj != nullptr) {
        setWindowTitle("Execute SQL for " + QString::fromStdString(emuDbProj->GetDb()->GetFileName()));
    }    
}