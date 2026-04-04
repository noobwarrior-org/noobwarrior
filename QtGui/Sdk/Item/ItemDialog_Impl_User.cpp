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
// File: ItemDialog_Impl_User.cpp
// Started by: Hattozo
// Started on: 4/3/2026
// Description:
#include "ItemDialog.h"
#include "Sdk/CreatorInfoWidget.h"

using namespace NoobWarrior;

void ItemDialog::User_AddFields() {
    std::string displayName;
    std::string status;
    std::string bio;
    int joinDate = 0;
    int lastOnline = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT DisplayName, Status, Bio, JoinDate, LastOnline FROM Id = ?");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            displayName = stmt.GetStringFromColumnIndex(0);
            status = stmt.GetStringFromColumnIndex(1);
            bio = stmt.GetStringFromColumnIndex(2);
            joinDate = stmt.GetIntFromColumnIndex(3);
            lastOnline = stmt.GetIntFromColumnIndex(4);
        }
    }

    mUser_DisplayNameInput = new QLineEdit(QString::fromStdString(displayName)); // fuck whoever made this qstring garbage
    mUser_DisplayNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Display Name", mUser_DisplayNameInput);

    mUser_StatusInput = new QLineEdit(QString::fromStdString(status));
    mUser_StatusInput->setPlaceholderText("What are you up to?");
    mContentLayout->addRow("Status", mUser_StatusInput);

    mUser_BioInput = new QLineEdit(QString::fromStdString(bio));
    mUser_BioInput->setPlaceholderText("Introduce yourself here");
    mContentLayout->addRow("Bio", mUser_BioInput);

    mUser_JoinDateInput = new QDateTimeEdit();
    mUser_JoinDateInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(joinDate) : QDateTime::currentDateTime());
    mContentLayout->addRow("Join Date", mUser_JoinDateInput);

    mUser_LastOnlineInput = new QDateTimeEdit();
    mUser_LastOnlineInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(lastOnline) : QDateTime::currentDateTime());
    mContentLayout->addRow("Last Online", mUser_LastOnlineInput);
}

bool ItemDialog::User_OnSave() {
    auto *db = GetDatabase();

    int id = mIdInput->text().toInt();
    std::string name = mNameInput->text().toStdString();
    std::string displayName = mUser_DisplayNameInput->text().toStdString();
    std::string status = mUser_StatusInput->text().toStdString();
    std::string bio = mUser_BioInput->text().toStdString();
    int joinDate = mUser_JoinDateInput->dateTime().toSecsSinceEpoch();
    int lastOnline = mUser_LastOnlineInput->dateTime().toSecsSinceEpoch();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO User (Id, Name, DisplayName, Status, Bio, JoinDate, LastOnline) VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            DisplayName = excluded.DisplayName,
            Status = excluded.Status,
            Bio = excluded.Bio,
            JoinDate = excluded.JoinDate,
            LastOnline = excluded.LastOnline;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, displayName);
    stmt.Bind(4, status);
    stmt.Bind(5, bio);
    stmt.Bind(6, joinDate);
    stmt.Bind(7, lastOnline);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
