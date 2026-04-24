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
// File: DirectConnectDialog.cpp
// Started by: Hattozo
// Started on: 4/24/2026
// Description:
#include "DirectConnectDialog.h"
#include "../Application.h"

using namespace NoobWarrior;

DirectConnectDialog::DirectConnectDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Direct Connect");
    InitWidgets();
}

void DirectConnectDialog::InitWidgets() {
    mLayout = new QVBoxLayout(this);

    auto *lol = new QLabel("Connect to IP Address", this);
    mLayout->addWidget(lol);

    mIpInput = new QLineEdit();
    mIpInput->setPlaceholderText("localhost:8080");
    mLayout->addWidget(mIpInput);

    mButtonBox = new QDialogButtonBox();
    mButtonBox->addButton("Join", QDialogButtonBox::AcceptRole);
    mButtonBox->addButton("Cancel", QDialogButtonBox::RejectRole);

    connect(mButtonBox, &QDialogButtonBox::accepted, [this]() {
        close();
        gApp->ConnectToServer("localhost", 8080);
    });

    connect(mButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });

    mLayout->addWidget(mButtonBox);
}