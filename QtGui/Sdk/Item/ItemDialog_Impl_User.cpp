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

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::User_AddFields() {
    std::string displayName;
    std::string status;
    std::string bio;
    std::string email;
    int64_t joinDate = 0;
    int64_t lastOnline = 0;
    int64_t placeVisits = 0;
    int64_t rank = 0;
    int64_t friendCount = 0;
    int64_t followersCount = 0;
    int64_t followingCount = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT DisplayName, Status, Bio, Email, JoinDate, LastOnline, PlaceVisits, Rank, Historical_FriendCount, Historical_FollowersCount, Historical_FollowingCount FROM User WHERE Id = ?");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            displayName = stmt.GetStringFromColumnIndex(0);
            status = stmt.GetStringFromColumnIndex(1);
            bio = stmt.GetStringFromColumnIndex(2);
            email = stmt.GetStringFromColumnIndex(3);
            joinDate = stmt.GetInt64FromColumnIndex(4);
            lastOnline = stmt.GetInt64FromColumnIndex(5);
            placeVisits = stmt.GetInt64FromColumnIndex(6);
            rank = stmt.GetInt64FromColumnIndex(7);
            friendCount = stmt.GetInt64FromColumnIndex(8);
            followersCount = stmt.GetInt64FromColumnIndex(9);
            followingCount = stmt.GetInt64FromColumnIndex(10);
        }
    }

    mUser_DisplayNameInput = new QLineEdit(QString::fromStdString(displayName));
    mUser_DisplayNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Display Name", mUser_DisplayNameInput);

    mUser_StatusInput = new QLineEdit(QString::fromStdString(status));
    mUser_StatusInput->setPlaceholderText("What are you up to?");
    mContentLayout->addRow("Status", mUser_StatusInput);

    mUser_BioInput = new QLineEdit(QString::fromStdString(bio));
    mUser_BioInput->setPlaceholderText("Introduce yourself here");
    mContentLayout->addRow("Bio", mUser_BioInput);

    mUser_EmailInput = new QLineEdit(QString::fromStdString(email));
    mUser_EmailInput->setPlaceholderText("user@example.com");
    mContentLayout->addRow("Email", mUser_EmailInput);

    mUser_JoinDateInput = new QDateTimeEdit();
    mUser_JoinDateInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(joinDate) : QDateTime::currentDateTime());
    mContentLayout->addRow("Join Date", mUser_JoinDateInput);

    mUser_LastOnlineInput = new QDateTimeEdit();
    mUser_LastOnlineInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(lastOnline) : QDateTime::currentDateTime());
    mContentLayout->addRow("Last Online", mUser_LastOnlineInput);

    mUser_RankInput = new QLineEdit(QString::number(rank));
    mUser_RankInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_RankInput));
    mContentLayout->addRow("Rank", mUser_RankInput);

    mUser_PlaceVisitsInput = new QLineEdit(QString::number(placeVisits));
    mUser_PlaceVisitsInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_PlaceVisitsInput));
    mContentLayout->addRow("Place Visits", mUser_PlaceVisitsInput);

    mUser_FriendCountInput = new QLineEdit(QString::number(friendCount));
    mUser_FriendCountInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_FriendCountInput));
    mContentLayout->addRow("Friends", mUser_FriendCountInput);

    mUser_FollowersCountInput = new QLineEdit(QString::number(followersCount));
    mUser_FollowersCountInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_FollowersCountInput));
    mContentLayout->addRow("Followers", mUser_FollowersCountInput);

    mUser_FollowingCountInput = new QLineEdit(QString::number(followingCount));
    mUser_FollowingCountInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_FollowingCountInput));
    mContentLayout->addRow("Following", mUser_FollowingCountInput);
}

bool ItemDialog::User_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string displayName = mUser_DisplayNameInput->text().toStdString();
    std::string status = mUser_StatusInput->text().toStdString();
    std::string bio = mUser_BioInput->text().toStdString();
    std::string email = mUser_EmailInput->text().toStdString();
    int64_t joinDate = mUser_JoinDateInput->dateTime().toSecsSinceEpoch();
    int64_t lastOnline = mUser_LastOnlineInput->dateTime().toSecsSinceEpoch();
    int64_t rank = mUser_RankInput->text().toLongLong();
    int64_t placeVisits = mUser_PlaceVisitsInput->text().toLongLong();
    int64_t friendCount = mUser_FriendCountInput->text().toLongLong();
    int64_t followersCount = mUser_FollowersCountInput->text().toLongLong();
    int64_t followingCount = mUser_FollowingCountInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO User (Id, Name, DisplayName, Status, Bio, Email, JoinDate, LastOnline, Rank, PlaceVisits, Historical_FriendCount, Historical_FollowersCount, Historical_FollowingCount) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            DisplayName = excluded.DisplayName,
            Status = excluded.Status,
            Bio = excluded.Bio,
            Email = excluded.Email,
            JoinDate = excluded.JoinDate,
            LastOnline = excluded.LastOnline,
            Rank = excluded.Rank,
            PlaceVisits = excluded.PlaceVisits,
            Historical_FriendCount = excluded.Historical_FriendCount,
            Historical_FollowersCount = excluded.Historical_FollowersCount,
            Historical_FollowingCount = excluded.Historical_FollowingCount;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, displayName);
    stmt.Bind(4, status);
    stmt.Bind(5, bio);
    stmt.Bind(6, email);
    stmt.Bind(7, joinDate);
    stmt.Bind(8, lastOnline);
    stmt.Bind(9, rank);
    stmt.Bind(10, placeVisits);
    stmt.Bind(11, friendCount);
    stmt.Bind(12, followersCount);
    stmt.Bind(13, followingCount);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
