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
// File: SelectStudioVersionDialog.cpp
// Started by: Hattozo
// Started on: 4/21/2026
// Description:
#include "SelectStudioVersionDialog.h"

using namespace NoobWarrior;

SelectStudioVersionDialog::SelectStudioVersionDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Select Studio Version");
    InitWidgets();
}

void SelectStudioVersionDialog::InitWidgets() {
    mLayout = new QVBoxLayout(this);

    mLabel = new QLabel("Choose the version of Studio that you would like to open.");
    mLabel->setWordWrap(true);
    mLayout->addWidget(mLabel);
}
