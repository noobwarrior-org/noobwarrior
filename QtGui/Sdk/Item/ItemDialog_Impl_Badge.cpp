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
// File: ItemDialog_Impl_Badge.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Badge type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Badge_AddFields() {
    AddOwnedItemFields();

    bool enabled = false;
    int64_t imageId = 0;
    int64_t awarded = 0;
    int64_t awardedYesterday = 0;
    int64_t universeId = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT ImageId, Enabled, Awarded, AwardedYesterday, UniverseId FROM Badge WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            imageId = stmt.GetInt64FromColumnIndex(0);
            enabled = stmt.GetIntFromColumnIndex(1);
            awarded = stmt.GetInt64FromColumnIndex(2);
            awardedYesterday = stmt.GetInt64FromColumnIndex(3);
            universeId = stmt.GetInt64FromColumnIndex(4);
        }
    }
    mImageIdInput->setText(QString::number(imageId));

    mBadge_EnabledInput = new QCheckBox();
    mBadge_EnabledInput->setChecked(enabled);
    mContentLayout->addRow("Enabled", mBadge_EnabledInput);

    // UniverseId is NOT NULL in the schema, so badges always belong to a universe.
    auto *universeFrame = new QFrame();
    auto *universeLayout = new QHBoxLayout(universeFrame);
    universeLayout->setContentsMargins(0, 0, 0, 0);
    mBadge_UniverseIdInput = new QLineEdit(QString::number(universeId));
    mBadge_UniverseIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBadge_UniverseIdInput));
    auto *universeBrowse = new QPushButton("Browse...");
    connect(universeBrowse, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Universe);
        if (id.has_value())
            mBadge_UniverseIdInput->setText(QString::number(id.value()));
    });
    universeLayout->addWidget(mBadge_UniverseIdInput);
    universeLayout->addWidget(universeBrowse);
    mContentLayout->addRow("Universe Id", universeFrame);

    mBadge_AwardedInput = new QLineEdit(QString::number(awarded));
    mBadge_AwardedInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBadge_AwardedInput));
    mContentLayout->addRow("Awarded", mBadge_AwardedInput);

    mBadge_AwardedYesterdayInput = new QLineEdit(QString::number(awardedYesterday));
    mBadge_AwardedYesterdayInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBadge_AwardedYesterdayInput));
    mContentLayout->addRow("Awarded Yesterday", mBadge_AwardedYesterdayInput);
}

bool ItemDialog::Badge_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->toPlainText().toStdString();
    int64_t imageId = mImageIdInput->text().toLongLong();
    bool enabled = mBadge_EnabledInput->isChecked();
    int64_t awarded = mBadge_AwardedInput->text().toLongLong();
    int64_t awardedYesterday = mBadge_AwardedYesterdayInput->text().toLongLong();
    int64_t universeId = mBadge_UniverseIdInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Badge (Id, Name, Description, Created, Updated, ImageId, Enabled, Awarded, AwardedYesterday, UniverseId) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            ImageId = excluded.ImageId,
            Enabled = excluded.Enabled,
            Awarded = excluded.Awarded,
            AwardedYesterday = excluded.AwardedYesterday,
            UniverseId = excluded.UniverseId;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, imageId);
    stmt.Bind(7, enabled);
    stmt.Bind(8, awarded);
    stmt.Bind(9, awardedYesterday);
    stmt.Bind(10, universeId);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
