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
// File: MasterLoginDialog.cpp
// Started by: Hattozo
// Started on: 7/2/2026
// Description:
#include "MasterLoginDialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

using namespace NoobWarrior;

MasterLoginDialog::MasterLoginDialog(QWidget* parent, const QString& title, const QString& tagline,
                                     const QString& defaultMasterUrl, bool allowGuests) : QDialog(parent) {
    setWindowTitle("Sign in to your master server");
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel(title.isEmpty() ? "Sign in to join" : title, this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel(tagline.isEmpty()
        ? "Sign in with any master server account. Your login is remembered and reused for every server you join."
        : tagline, this));

    layout->addWidget(new QLabel("Master server", this));
    mMasterUrl = new QLineEdit(this);
    mMasterUrl->setPlaceholderText("https://master.example.com");
    mMasterUrl->setText(defaultMasterUrl);
    layout->addWidget(mMasterUrl);

    layout->addWidget(new QLabel("Username", this));
    mUsername = new QLineEdit(this);
    layout->addWidget(mUsername);

    layout->addWidget(new QLabel("Password", this));
    mPassword = new QLineEdit(this);
    mPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(mPassword);

    auto* buttons = new QDialogButtonBox(this);
    QPushButton* loginBtn = buttons->addButton("Sign in", QDialogButtonBox::AcceptRole);
    connect(loginBtn, &QPushButton::clicked, this, [this]() {
        if (mMasterUrl->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Sign in", "Enter your master server address.");
            return;
        }
        if (mUsername->text().trimmed().isEmpty() || mPassword->text().isEmpty()) {
            QMessageBox::warning(this, "Sign in", "Enter a username and password.");
            return;
        }
        mMode = Mode::Password;
        accept();
    });
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

QString MasterLoginDialog::MasterUrl() const { return mMasterUrl ? mMasterUrl->text().trimmed() : QString(); }
QString MasterLoginDialog::Username() const { return mUsername ? mUsername->text().trimmed() : QString(); }
QString MasterLoginDialog::Password() const { return mPassword ? mPassword->text() : QString(); }
