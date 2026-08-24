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
// File: AddMasterServerDialog.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description: Prompt for a master server address, resolving its branding before it is added
#include "AddMasterServerDialog.h"
#include "MasterHttp.h"

#include <nlohmann/json.hpp>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace NoobWarrior;

AddMasterServerDialog::AddMasterServerDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Add Master Server");
    setModal(true);
    InitWidgets();
}

void AddMasterServerDialog::InitWidgets() {
    auto *layout = new QVBoxLayout(this);

    auto *titleLabel = new QLabel("Add a master server", this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel(
        "Master servers list the game servers people are running. They also host a workshop you can\n"
        "download databases and plugins from. You can browse one without an account.", this));

    layout->addWidget(new QLabel("Address", this));
    mUrlInput = new QLineEdit(this);
    mUrlInput->setPlaceholderText("master.example.com");
    layout->addWidget(mUrlInput);

    mStatusLabel = new QLabel(this);
    mStatusLabel->setWordWrap(true);
    layout->addWidget(mStatusLabel);

    auto *buttons = new QDialogButtonBox(this);
    mAddButton = buttons->addButton("Add", QDialogButtonBox::AcceptRole);
    buttons->addButton("Cancel", QDialogButtonBox::RejectRole);
    connect(mAddButton, &QPushButton::clicked, this, &AddMasterServerDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(420, sizeHint().height());
}

void AddMasterServerDialog::accept() {
    Resolve();
}

void AddMasterServerDialog::SetBusy(bool busy) {
    mAddButton->setEnabled(!busy);
    mUrlInput->setEnabled(!busy);
}

void AddMasterServerDialog::Resolve() {
    QString url = MasterServerStore::NormalizeUrl(mUrlInput->text());
    if (url.isEmpty()) {
        mStatusLabel->setText("Enter a master server address.");
        return;
    }

    if (MasterServerStore::Find(url).has_value()) {
        mStatusLabel->setText("That master server is already in your list.");
        return;
    }

    SetBusy(true);
    mStatusLabel->setText("Contacting " + MasterServerStore::HostOf(url) + "...");

    MasterHttp::Get(this, url, "/fed/v1/info", [this, url](const MasterResponse &response) {
        SetBusy(false);

        if (!response.Ok) {
            mStatusLabel->setText("Couldn't reach that master server: " +
                                  QString::fromStdString(response.Error));
            return;
        }

        nlohmann::json info = nlohmann::json::parse(response.Body, nullptr, false);
        if (info.is_discarded() || !info.is_object()) {
            mStatusLabel->setText("That address answered, but it isn't a noobWarrior master server.");
            return;
        }

        mResult.Url = url;
        mResult.Name = QString::fromStdString(info.value("Name", std::string{}));
        mResult.Domain = QString::fromStdString(info.value("Domain", std::string{}));
        mResult.Tagline = QString::fromStdString(info.value("Tagline", std::string{}));
        if (mResult.Name.isEmpty())
            mResult.Name = MasterServerStore::HostOf(url);

        // Straight to the base class: accept() is our "try to resolve" entry point.
        QDialog::accept();
    });
}
