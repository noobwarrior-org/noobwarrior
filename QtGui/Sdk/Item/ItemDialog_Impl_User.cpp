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
#include "ItemOpenSaveDialog.h"
#include "Application.h"
#include "Sdk/CreatorInfoWidget.h"

#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/UserRank.h>

#include <QRegularExpressionValidator>
#include <QDoubleValidator>

using namespace NoobWarrior;

void ItemDialog::User_AddFields() {
    std::string displayName;
    std::string status;
    std::string bio;
    std::string email;
    int64_t joinDate = 0;
    int64_t lastOnline = 0;
    int64_t placeVisits = 0;
    int64_t rank = kUserRankDefault;
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
            // A NULL Rank means "never set", which resolves to the default rank - not to 0.
            // Reading it as 0 would show every existing account as a Guest, and saving would then
            // persist that 0 as a deliberate choice. AuthUtil::ResolveStoredRank does the same on
            // the web side; the two must agree or the SDK and the site disagree about who somebody is.
            if (!stmt.IsColumnIndexNull(7))
                rank = ClampUserRank(stmt.GetInt64FromColumnIndex(7));
            friendCount = stmt.GetInt64FromColumnIndex(8);
            followersCount = stmt.GetInt64FromColumnIndex(9);
            followingCount = stmt.GetInt64FromColumnIndex(10);
        }
    }

    AddSectionHeader("Profile");

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
    
    AddSectionHeader("Permissions");
    mUser_RankInput = new QComboBox();
    bool rankIsNamed = false;
    if (Registry* reg = gApp->GetCore()->GetRegistry()) {
        if (std::optional<sol::table> roles = reg->GetKeyValue<sol::table>("emu.roles")) {
            for (size_t i = 1; i <= roles->size(); i++) {
                sol::optional<sol::table> role = roles->get<sol::optional<sol::table>>(i);
                if (!role.has_value())
                    continue;
                const int64_t roleRank = role->get_or("rank", static_cast<int64_t>(0));
                const std::string roleName = role->get_or<std::string>("name", "");
                if (roleName.empty())
                    continue;
                // Rank 0 is the rank of having no account at all - it is what an unauthenticated
                // visitor is judged as, which is why emu.permissions.connect_to_server defaults to
                // it. A row in the User table is by definition an account, so 0 is not offered here.
                // It is still shown if some row already holds it, so opening and saving such a user
                // does not quietly move them.
                if (roleRank <= kUserRankGuest && rank != roleRank)
                    continue;
                mUser_RankInput->addItem(QString("%1 (%2)").arg(QString::fromStdString(roleName)).arg(roleRank),
                                         QVariant::fromValue(roleRank));
                if (roleRank == rank)
                    rankIsNamed = true;
            }
        }
    }
    if (!rankIsNamed)
        mUser_RankInput->addItem(QString("Rank %1").arg(rank), QVariant::fromValue(rank));

    if (const int rankIdx = mUser_RankInput->findData(QVariant::fromValue(rank)); rankIdx != -1)
        mUser_RankInput->setCurrentIndex(rankIdx);
    mContentLayout->addRow("Rank", mUser_RankInput);

    AddSectionHeader("Statistics");

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

    // ---- Character appearance (Character* columns + UserCharacterItem table) ----
    int64_t characterBodyType = 0;
    double characterWidth = 1.0;
    double characterHeight = 1.0;
    double characterHead = 1.0;
    double characterProportions = 0.0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT CharacterBodyType, CharacterWidth, CharacterHeight, CharacterHead, CharacterProportions FROM User WHERE Id = ?");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            characterBodyType = stmt.GetInt64FromColumnIndex(0);
            characterWidth = stmt.GetDoubleFromColumnIndex(1);
            characterHeight = stmt.GetDoubleFromColumnIndex(2);
            characterHead = stmt.GetDoubleFromColumnIndex(3);
            characterProportions = stmt.GetDoubleFromColumnIndex(4);
        }
    }

    AddSectionHeader("Character");

    mUser_CharacterBodyTypeInput = new QLineEdit(QString::number(characterBodyType));
    mUser_CharacterBodyTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUser_CharacterBodyTypeInput));
    mContentLayout->addRow("Body Type", mUser_CharacterBodyTypeInput);

    mUser_CharacterWidthInput = new QLineEdit(QString::number(characterWidth));
    mUser_CharacterWidthInput->setValidator(new QDoubleValidator(mUser_CharacterWidthInput));
    mContentLayout->addRow("Width", mUser_CharacterWidthInput);

    mUser_CharacterHeightInput = new QLineEdit(QString::number(characterHeight));
    mUser_CharacterHeightInput->setValidator(new QDoubleValidator(mUser_CharacterHeightInput));
    mContentLayout->addRow("Height", mUser_CharacterHeightInput);

    mUser_CharacterHeadInput = new QLineEdit(QString::number(characterHead));
    mUser_CharacterHeadInput->setValidator(new QDoubleValidator(mUser_CharacterHeadInput));
    mContentLayout->addRow("Head", mUser_CharacterHeadInput);

    mUser_CharacterProportionsInput = new QLineEdit(QString::number(characterProportions));
    mUser_CharacterProportionsInput->setValidator(new QDoubleValidator(mUser_CharacterProportionsInput));
    mContentLayout->addRow("Proportions", mUser_CharacterProportionsInput);

    // The assets the user is currently wearing (UserCharacterItem table).
    auto *itemFrame = new QFrame();
    auto *itemFrameLayout = new QVBoxLayout(itemFrame);
    itemFrameLayout->setContentsMargins(0, 0, 0, 0);

    mUser_CharacterItemList = new ItemListWidget(nullptr, GetDatabase());
    mUser_CharacterItemList->SetOnContextMenuShown([this](QMenu* menu, ItemWidget* item) {
        QAction* removeAction = menu->addAction(QIcon(":/images/silk/cross.png"), "Remove From List");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int64_t assetId = item->GetId();
            mUser_CharacterItemList->Remove(item->GetType(), assetId);
            mUser_PendingDeleteCharacterItems.push_back(assetId);
            mUser_PendingCharacterItems.removeAll(assetId);
        });
    });

    if (mId.has_value()) {
        Statement itemsStmt = GetDatabase()->PrepareStatement("SELECT AssetId FROM UserCharacterItem WHERE Id = ?;");
        itemsStmt.Bind(1, mId.value());
        while (itemsStmt.Step() == SQLITE_ROW) {
            int64_t assetId = itemsStmt.GetInt64FromColumnIndex(0);
            if (!mUser_CharacterItemList->Add(ItemType::Asset, assetId))
                QMessageBox::critical(this, "Error", "Failed to load asset into list!");
        }
    }

    mUser_AddCharacterItemButton = new QPushButton("Add Asset");
    connect(mUser_AddCharacterItemButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset);
        if (id.has_value()) {
            if (mUser_CharacterItemList->IsItemInList(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Asset Already Exists", "You already added this asset to the list.");
                return;
            }
            if (!mUser_CharacterItemList->Add(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Error", "Failed to add asset");
                return;
            }
            mUser_PendingCharacterItems.push_back(id.value());
            mUser_PendingDeleteCharacterItems.removeAll(id.value());
        }
    });

    itemFrameLayout->addWidget(mUser_CharacterItemList);
    itemFrameLayout->addWidget(mUser_AddCharacterItemButton);
    mContentLayout->addRow(itemFrame);
}

static constexpr const char* sUserChildTables[] = {
    "UserCharacterItem",
    "UserCharacterBodyColor",
    "UserNames",
    "UserGroups",
    "UserInventory",
    "UserFavorites",
    "UserLikesDislikes"
};

bool ItemDialog::User_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    int64_t oldId = mId.has_value() ? mId.value() : id;
    bool idChanged = mId.has_value() && oldId != id;

    if (idChanged) {
        // The main row was already renamed by ItemDialog::OnSave; bring the user-owned child tables along.
        for (int i = 0; i < std::size(sUserChildTables); i++) {
            Statement updateStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", sUserChildTables[i]));
            updateStmt.Bind(1, id);
            updateStmt.Bind(2, oldId);
            if (updateStmt.Step() != SQLITE_DONE) {
                QMessageBox::critical(this, "Cannot Save",
                    QString("Failed to update Id field in %1 table.\nLast error: %2")
                    .arg(sUserChildTables[i])
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
                return false;
            }
        }
    }

    std::string name = mNameInput->text().toStdString();
    std::string displayName = mUser_DisplayNameInput->text().toStdString();
    std::string status = mUser_StatusInput->text().toStdString();
    std::string bio = mUser_BioInput->text().toStdString();
    std::string email = mUser_EmailInput->text().toStdString();
    int64_t joinDate = mUser_JoinDateInput->dateTime().toSecsSinceEpoch();
    int64_t lastOnline = mUser_LastOnlineInput->dateTime().toSecsSinceEpoch();
    int64_t rank = mUser_RankInput->currentData().toLongLong();
    int64_t placeVisits = mUser_PlaceVisitsInput->text().toLongLong();
    int64_t friendCount = mUser_FriendCountInput->text().toLongLong();
    int64_t followersCount = mUser_FollowersCountInput->text().toLongLong();
    int64_t followingCount = mUser_FollowingCountInput->text().toLongLong();
    int64_t characterBodyType = mUser_CharacterBodyTypeInput->text().toLongLong();
    double characterWidth = mUser_CharacterWidthInput->text().toDouble();
    double characterHeight = mUser_CharacterHeightInput->text().toDouble();
    double characterHead = mUser_CharacterHeadInput->text().toDouble();
    double characterProportions = mUser_CharacterProportionsInput->text().toDouble();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO User (Id, Name, DisplayName, Status, Bio, Email, JoinDate, LastOnline, Rank, PlaceVisits, Historical_FriendCount, Historical_FollowersCount, Historical_FollowingCount, CharacterBodyType, CharacterWidth, CharacterHeight, CharacterHead, CharacterProportions) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
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
            Historical_FollowingCount = excluded.Historical_FollowingCount,
            CharacterBodyType = excluded.CharacterBodyType,
            CharacterWidth = excluded.CharacterWidth,
            CharacterHeight = excluded.CharacterHeight,
            CharacterHead = excluded.CharacterHead,
            CharacterProportions = excluded.CharacterProportions;
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
    stmt.Bind(14, characterBodyType);
    stmt.Bind(15, characterWidth);
    stmt.Bind(16, characterHeight);
    stmt.Bind(17, characterHead);
    stmt.Bind(18, characterProportions);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    for (int64_t assetId : mUser_PendingDeleteCharacterItems) {
        Statement del = db->PrepareStatement("DELETE FROM UserCharacterItem WHERE Id = ? AND AssetId = ?;");
        del.Bind(1, id);
        del.Bind(2, assetId);
        if (del.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    for (int64_t assetId : mUser_PendingCharacterItems) {
        Statement ins = db->PrepareStatement("INSERT OR IGNORE INTO UserCharacterItem (Id, AssetId) VALUES (?, ?);");
        ins.Bind(1, id);
        ins.Bind(2, assetId);
        if (ins.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}
