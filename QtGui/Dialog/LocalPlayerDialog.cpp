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
// File: LocalPlayerDialog.cpp
// Started by: Hattozo
// Started on: 4/23/2026
// Description:
#include "LocalPlayerDialog.h"
#include "../Application.h"

#include <QRegularExpressionValidator>
#include <QRegularExpression>

using namespace NoobWarrior;

LocalPlayerDialog::LocalPlayerDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Local Player Settings");
    InitWidgets();
}

LocalPlayerDialog::~LocalPlayerDialog() {}

void LocalPlayerDialog::InitWidgets() {
    Registry* reg = gApp->GetCore()->GetRegistry();

    mLayout = new QHBoxLayout(this);
    mSideLayout = new QVBoxLayout;
    mOtherSideLayout = new QVBoxLayout;
    mFormLayout = new QFormLayout;

    mIdInput = new QLineEdit;
    mNameInput = new QLineEdit;
    mDisplayNameInput = new QLineEdit;

    mIdInput->setText(QString::number(reg->GetKeyValue<int64_t>("user.id").value_or(5)));
    mNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.name").value_or("Player")));
    mDisplayNameInput->setText(QString::fromStdString(reg->GetKeyValue<std::string>("user.display_name").value_or("Player")));

    mSideLayout->addLayout(mFormLayout);
    mFormLayout->addRow("User Id", mIdInput);
    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mFormLayout->addRow("Name", mNameInput);
    mFormLayout->addRow("Display Name", mDisplayNameInput);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, [this, reg]() {
        reg->SetKeyValue("user.id", mIdInput->text().toLongLong());
        reg->SetKeyValue("user.name", mNameInput->text().toStdString());
        reg->SetKeyValue("user.display_name", mDisplayNameInput->text().toStdString());
        close();
    });
    connect(mButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });
    mOtherSideLayout->addWidget(mButtonBox);

    setLayout(mLayout);
    mLayout->addLayout(mSideLayout);
    mLayout->addLayout(mOtherSideLayout);
}
