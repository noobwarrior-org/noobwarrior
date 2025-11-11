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

        RobloxAccount *acc;
        AuthResponse res = gApp->GetCore()->GetRobloxAuth()->AddAccountFromToken(token.toStdString(), &acc);
        if (acc != nullptr)
            gApp->GetCore()->GetRobloxAuth()->SetActiveAccount(acc);

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