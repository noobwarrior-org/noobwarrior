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
// File: AuthTokenDialog.cpp
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include "AuthTokenDialog.h"
#include "../Settings/AccountPage.h"
#include "../Application.h"

using namespace NoobWarrior;

QString AuthTokenDialog::GetSourceURL() {
    return "https://github.com/noobwarrior-org/noobwarrior/blob/master/QtGui/Dialog/AuthTokenDialog.cpp";
}

AuthTokenDialog::AuthTokenDialog(QWidget *parent) : AuthBaseDialog(parent) {
    InitWidgets();
}

void AuthTokenDialog::InitWidgets() {
    TitleLabel = new QLabel("Authenticate your Roblox account with your .ROBLOSECURITY cookie", this);
    Layout.addWidget(TitleLabel);

    TokenInput = new QLineEdit(this);
    TokenInput->setPlaceholderText(".ROBLOSECURITY Token");
    Layout.addWidget(TokenInput);

    // init the rest of the box, now they're not null pointers
    AuthBaseDialog::InitWidgets();

    connect(ButtonBox, &QDialogButtonBox::accepted, [this]() {
        QString token = TokenInput->text();

        Account *acc;
        AuthResponse res = gApp->GetCore()->GetRbxKeychain()->AddAccountFromToken(token.toStdString(), &acc);
        if (acc != nullptr)
            gApp->GetCore()->GetRbxKeychain()->SetActiveAccount(acc);

        auto accPage = dynamic_cast<AccountPage*>(parent());
        if (accPage != nullptr) {
            accPage->Refresh();
        }

        close();
    });
    connect(ButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });
}