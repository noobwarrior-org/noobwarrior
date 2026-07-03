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
// File: ServerLoginDialog.cpp
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#include "ServerLoginDialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

using namespace NoobWarrior;

ServerLoginDialog::ServerLoginDialog(QWidget* parent, const QString& title, const QString& tagline,
                                     bool passwordBased, bool allowGuests) : QDialog(parent) {
    setWindowTitle("Log in");
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel(title.isEmpty() ? "This server requires authentication" : title, this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    if (!tagline.isEmpty())
        layout->addWidget(new QLabel(tagline, this));

    if (passwordBased) {
        layout->addWidget(new QLabel("Username", this));
        mUsername = new QLineEdit(this);
        layout->addWidget(mUsername);

        layout->addWidget(new QLabel("Password", this));
        mPassword = new QLineEdit(this);
        mPassword->setEchoMode(QLineEdit::Password);
        layout->addWidget(mPassword);
    } else {
        layout->addWidget(new QLabel("This server uses OAuth2 login, which isn't supported here yet.", this));
    }

    auto* buttons = new QDialogButtonBox(this);
    if (passwordBased) {
        QPushButton* loginBtn = buttons->addButton("Log in", QDialogButtonBox::AcceptRole);
        connect(loginBtn, &QPushButton::clicked, this, [this]() {
            if (mUsername->text().trimmed().isEmpty() || mPassword->text().isEmpty()) {
                QMessageBox::warning(this, "Log in", "Enter a username and password.");
                return;
            }
            mMode = Mode::Password;
            accept();
        });
    }
    if (allowGuests) {
        QPushButton* guestBtn = buttons->addButton("Play as Guest", QDialogButtonBox::ActionRole);
        connect(guestBtn, &QPushButton::clicked, this, [this]() {
            mMode = Mode::Guest;
            accept();
        });
    }
    buttons->addButton("Cancel", QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QString ServerLoginDialog::Username() const { return mUsername ? mUsername->text().trimmed() : QString(); }
QString ServerLoginDialog::Password() const { return mPassword ? mPassword->text() : QString(); }
