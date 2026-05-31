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
// File: ItemDialog_Impl_Group.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Group type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Group_AddFields() {
    auto *db = GetDatabase();

    // The Group table has no Updated column, so AddOwnedItemFields can't be reused; build the
    // owned-item fields by hand (reusing the shared Description/Created widgets).
    std::string description;
    int64_t created = 0;
    int64_t ownerId = 0;
    int64_t imageId = 0;
    int64_t funds = 0;
    std::string shout;
    bool enemyDeclarationsEnabled = false;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Description, Created, OwnerId, ImageId, Funds, Shout, EnemyDeclarationsEnabled FROM \"Group\" WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            description = stmt.GetStringFromColumnIndex(0);
            created = stmt.GetInt64FromColumnIndex(1);
            ownerId = stmt.GetInt64FromColumnIndex(2);
            imageId = stmt.GetInt64FromColumnIndex(3);
            funds = stmt.GetInt64FromColumnIndex(4);
            shout = stmt.GetStringFromColumnIndex(5);
            enemyDeclarationsEnabled = stmt.GetIntFromColumnIndex(6);
        }
    }
    mImageIdInput->setText(QString::number(imageId));

    mOwned_DescriptionInput = new QLineEdit(QString::fromStdString(description));
    mOwned_DescriptionInput->setPlaceholderText("Describe your group here");
    mContentLayout->addRow("Description", mOwned_DescriptionInput);

    mOwned_CreatedInput = new QDateTimeEdit();
    mOwned_CreatedInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(created) : QDateTime::currentDateTime());
    mContentLayout->addRow("Created", mOwned_CreatedInput);

    // OwnerId references a User.
    auto *ownerFrame = new QFrame();
    auto *ownerLayout = new QHBoxLayout(ownerFrame);
    ownerLayout->setContentsMargins(0, 0, 0, 0);
    mGroup_OwnerIdInput = new QLineEdit(QString::number(ownerId));
    mGroup_OwnerIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mGroup_OwnerIdInput));
    auto *ownerBrowse = new QPushButton("Browse...");
    connect(ownerBrowse, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::User);
        if (id.has_value())
            mGroup_OwnerIdInput->setText(QString::number(id.value()));
    });
    ownerLayout->addWidget(mGroup_OwnerIdInput);
    ownerLayout->addWidget(ownerBrowse);
    mContentLayout->addRow("Owner Id", ownerFrame);

    mGroup_FundsInput = new QLineEdit(QString::number(funds));
    mGroup_FundsInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mGroup_FundsInput));
    mContentLayout->addRow("Funds", mGroup_FundsInput);

    mGroup_ShoutInput = new QLineEdit(QString::fromStdString(shout));
    mGroup_ShoutInput->setPlaceholderText("Group shout");
    mContentLayout->addRow("Shout", mGroup_ShoutInput);

    mGroup_EnemyDeclarationsEnabledInput = new QCheckBox();
    mGroup_EnemyDeclarationsEnabledInput->setChecked(enemyDeclarationsEnabled);
    mContentLayout->addRow("Enemy Declarations", mGroup_EnemyDeclarationsEnabledInput);
}

static constexpr const char* sGroupChildTables[] = {
    "GroupRole",
    "GroupWall",
    "GroupLog",
    "GroupAlly",
    "GroupEnemy",
    "GroupHistorical",
    "GroupSocialLink"
};

bool ItemDialog::Group_OnSave() {
    auto *db = GetDatabase();

    int64_t newId = mIdInput->text().toLongLong();
    int64_t oldId = mId.has_value() ? mId.value() : newId;
    bool idChanged = mId.has_value() && oldId != newId;

    if (idChanged) {
        for (int i = 0; i < std::size(sGroupChildTables); i++) {
            Statement updateStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", sGroupChildTables[i]));
            updateStmt.Bind(1, newId);
            updateStmt.Bind(2, oldId);
            if (updateStmt.Step() != SQLITE_DONE) {
                QMessageBox::critical(this, "Cannot Save",
                    QString("Failed to update Id field in %1 table.\nLast error: %2")
                    .arg(sGroupChildTables[i])
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
                return false;
            }
        }
    }

    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->text().toStdString();
    int64_t created = mOwned_CreatedInput->dateTime().toSecsSinceEpoch();
    int64_t ownerId = mGroup_OwnerIdInput->text().toLongLong();
    int64_t imageId = mImageIdInput->text().toLongLong();
    int64_t funds = mGroup_FundsInput->text().toLongLong();
    std::string shout = mGroup_ShoutInput->text().toStdString();
    bool enemyDeclarationsEnabled = mGroup_EnemyDeclarationsEnabledInput->isChecked();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO "Group" (Id, Name, Description, Created, OwnerId, ImageId, Funds, Shout, EnemyDeclarationsEnabled) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            OwnerId = excluded.OwnerId,
            ImageId = excluded.ImageId,
            Funds = excluded.Funds,
            Shout = excluded.Shout,
            EnemyDeclarationsEnabled = excluded.EnemyDeclarationsEnabled;
    )");
    stmt.Bind(1, newId);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, created);
    stmt.Bind(5, ownerId);
    stmt.Bind(6, imageId);
    stmt.Bind(7, funds);
    stmt.Bind(8, shout);
    stmt.Bind(9, enemyDeclarationsEnabled);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
