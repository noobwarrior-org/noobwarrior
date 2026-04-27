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
// File: ItemDialog_Impl_Universe.cpp
// Started by: Hattozo
// Started on: 4/19/2026
// Description:
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <NoobWarrior/Roblox/Api/User.h>

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Universe_AddFields() {
    auto *db = GetDatabase();

    mOwned_CreatedInput = new QDateTimeEdit();
    mOwned_CreatedInput->setDateTime(QDateTime::currentDateTime());
    mContentLayout->addRow("Created", mOwned_CreatedInput);

    mOwned_UpdatedInput = new QDateTimeEdit();
    mOwned_UpdatedInput->setDateTime(QDateTime::currentDateTime());
    mContentLayout->addRow("Updated", mOwned_UpdatedInput);

    mOwned_CreatorInfoWidget = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", mOwned_CreatorInfoWidget);

    if (mId.has_value()) {
        // deserialization
        Statement stmt = db->PrepareStatement(std::format("SELECT Created, Updated FROM {} WHERE Id = ?;", GetTableNameFromItemType(mType)));
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            mOwned_CreatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(0)));
            mOwned_UpdatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(1)));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }

    int64_t startPlaceId = 0;
    int64_t userId = 0;
    int64_t groupId = 0;
    bool active = false;
    int64_t visits = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT StartPlaceId, UserId, GroupId, Active, Visits FROM Universe WHERE Id = ?");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            startPlaceId = stmt.GetInt64FromColumnIndex(0);
            userId = stmt.GetInt64FromColumnIndex(1);
            groupId = stmt.GetInt64FromColumnIndex(2);
            active = stmt.GetIntFromColumnIndex(3);
            visits = stmt.GetInt64FromColumnIndex(4);
        }
    }

    mUniverse_StartPlaceIdInput = new QLineEdit();
    mUniverse_StartPlaceIdInput->setText(QString::number(startPlaceId));
    mUniverse_StartPlaceIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_StartPlaceIdInput));
    mContentLayout->addRow("Start Place Id", mUniverse_StartPlaceIdInput);

    mUniverse_VisitsInput = new QLineEdit();
    mUniverse_VisitsInput->setText(QString::number(visits));
    mUniverse_VisitsInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_VisitsInput));
    mContentLayout->addRow("Visits", mUniverse_VisitsInput);

    mUniverse_ActiveInput = new QCheckBox();
    mUniverse_ActiveInput->setChecked(active);
    mContentLayout->addRow("Active", mUniverse_ActiveInput);

    mUniverse_PlaceFrame = new QFrame();
    auto *placeFrameLayout = new QVBoxLayout(mUniverse_PlaceFrame);

    mUniverse_PlaceList = new QListWidget();
    mUniverse_AddPlaceButton = new QPushButton("Add Place");
    placeFrameLayout->addWidget(mUniverse_PlaceList);
    placeFrameLayout->addWidget(mUniverse_AddPlaceButton);

    connect(mUniverse_AddPlaceButton, &QPushButton::clicked, [this]() {
        int64_t id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Place, true);
    });

    mContentLayout->addRow("Places", mUniverse_PlaceFrame);

    /*
    mUniverse_CreatorTypeInput = new QComboBox();
    for (int i = 0; i < 2; i++) {
        Roblox::CreatorType type = static_cast<Roblox::CreatorType>(i);
        mUniverse_CreatorTypeInput->addItem(QString::fromStdString(Roblox::CreatorTypeAsTranslatableString(type)), type);
    }
    mContentLayout->addRow("Creator Type", mUniverse_CreatorTypeInput);
    */
}

static constexpr const char* sTablesThatNeedToBeUpdated[] = {
    "UniversePlace",
    "UniverseMisc",
    "UniverseHistorical",
    "UniverseSocialLink"
};

bool ItemDialog::Universe_OnSave() {
    auto *db = GetDatabase();

    int64_t newId = mIdInput->text().toLongLong();
    int64_t oldId = mId.has_value() ? mId.value() : newId;
    bool idChanged = mId.has_value() && oldId != newId;

    if (idChanged) {
        // id changed, pls update these other tables!!!
        for (int i = 0; i < std::size(sTablesThatNeedToBeUpdated); i++) {
            Statement updateStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", sTablesThatNeedToBeUpdated[i]));
            updateStmt.Bind(1, newId);
            updateStmt.Bind(2, oldId);
            if (updateStmt.Step() != SQLITE_DONE) {
                QMessageBox::critical(this, "Cannot Save",
                    QString("Failed to update Id field in %1 table.\nLast error: %2")
                    .arg(sTablesThatNeedToBeUpdated[i])
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
                return false;
            }
        }
    }

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    int64_t startPlaceId = mUniverse_StartPlaceIdInput->text().toLongLong();
    int64_t userId = 0;
    int64_t groupId = 0;
    bool active = mUniverse_ActiveInput->isChecked();
    int64_t visits = mUniverse_VisitsInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Universe (Id, Name, Created, Updated, StartPlaceId, UserId, GroupId, Active, Visits) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Created = excluded.Created,
            Updated = excluded.Updated,
            StartPlaceId = excluded.StartPlaceId,
            UserId = excluded.UserId,
            GroupId = excluded.GroupId,
            Active = excluded.Active,
            Visits = excluded.Visits;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, mOwned_CreatedInput->dateTime().toSecsSinceEpoch());
    stmt.Bind(4, mOwned_UpdatedInput->dateTime().toSecsSinceEpoch());
    stmt.Bind(5, startPlaceId);
    stmt.Bind(6, userId);
    stmt.Bind(7, groupId);
    stmt.Bind(8, active);
    stmt.Bind(9, visits);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
