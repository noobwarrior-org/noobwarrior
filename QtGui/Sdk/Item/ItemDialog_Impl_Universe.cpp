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

    AddSectionHeader("Details");

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

    std::optional<int64_t> startPlaceId = std::nullopt;
    int64_t userId = 0;
    int64_t groupId = 0;
    bool active = false;
    int64_t visits = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT StartPlaceId, UserId, GroupId, Active, Visits FROM Universe WHERE Id = ?");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            !stmt.IsColumnIndexNull(0) ?
                startPlaceId = stmt.GetInt64FromColumnIndex(0) :
                startPlaceId = std::nullopt;
            userId = stmt.GetInt64FromColumnIndex(1);
            groupId = stmt.GetInt64FromColumnIndex(2);
            active = stmt.GetIntFromColumnIndex(3);
            visits = stmt.GetInt64FromColumnIndex(4);
        }
    }

    mUniverse_StartPlaceId = startPlaceId;

    AddSectionHeader("Statistics");

    mUniverse_VisitsInput = new QLineEdit();
    mUniverse_VisitsInput->setText(QString::number(visits));
    mUniverse_VisitsInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_VisitsInput));
    mContentLayout->addRow("Visits", mUniverse_VisitsInput);

    int64_t favorites = 0;
    int64_t likes = 0;
    int64_t dislikes = 0;
    if (mId.has_value()) {
        Statement histStmt = db->PrepareStatement("SELECT Favorites, Likes, Dislikes FROM UniverseHistorical WHERE Id = ?");
        histStmt.Bind(1, mId.value());
        if (histStmt.Step() == SQLITE_ROW) {
            favorites = histStmt.GetInt64FromColumnIndex(0);
            likes = histStmt.GetInt64FromColumnIndex(1);
            dislikes = histStmt.GetInt64FromColumnIndex(2);
        }
    }

    mUniverse_FavoritesInput = new QLineEdit(QString::number(favorites));
    mUniverse_FavoritesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_FavoritesInput));
    mContentLayout->addRow("Favorites", mUniverse_FavoritesInput);

    mUniverse_LikesInput = new QLineEdit(QString::number(likes));
    mUniverse_LikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_LikesInput));
    mContentLayout->addRow("Likes", mUniverse_LikesInput);

    mUniverse_DislikesInput = new QLineEdit(QString::number(dislikes));
    mUniverse_DislikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_DislikesInput));
    mContentLayout->addRow("Dislikes", mUniverse_DislikesInput);

    // Settings live partly in the Universe table (Active) and partly in UniverseMisc.
    int64_t genre = 0;
    int64_t subgenre = 0;
    int64_t avatarType = 0;
    int64_t accessType = 0;
    int64_t paymentType = 0;
    int64_t ageRating = 0;
    bool allowPrivateServers = false;
    bool allowDirectAccess = false;
    std::string supportedDevices;
    if (mId.has_value()) {
        Statement miscStmt = db->PrepareStatement("SELECT Genre, Subgenre, AvatarType, AccessType, PaymentType, AllowPrivateServers, AllowDirectAccessToPlaces, AgeRating, SupportedDevices FROM UniverseMisc WHERE Id = ?");
        miscStmt.Bind(1, mId.value());
        if (miscStmt.Step() == SQLITE_ROW) {
            genre = miscStmt.GetInt64FromColumnIndex(0);
            subgenre = miscStmt.GetInt64FromColumnIndex(1);
            avatarType = miscStmt.GetInt64FromColumnIndex(2);
            accessType = miscStmt.GetInt64FromColumnIndex(3);
            paymentType = miscStmt.GetInt64FromColumnIndex(4);
            allowPrivateServers = miscStmt.GetIntFromColumnIndex(5);
            allowDirectAccess = miscStmt.GetIntFromColumnIndex(6);
            ageRating = miscStmt.GetInt64FromColumnIndex(7);
            supportedDevices = miscStmt.GetStringFromColumnIndex(8);
        }
    }

    AddSectionHeader("Settings");

    mUniverse_ActiveInput = new QCheckBox();
    mUniverse_ActiveInput->setChecked(active);
    mContentLayout->addRow("Active", mUniverse_ActiveInput);

    mUniverse_GenreInput = new QLineEdit(QString::number(genre));
    mUniverse_GenreInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_GenreInput));
    mContentLayout->addRow("Genre", mUniverse_GenreInput);

    mUniverse_SubgenreInput = new QLineEdit(QString::number(subgenre));
    mUniverse_SubgenreInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_SubgenreInput));
    mContentLayout->addRow("Subgenre", mUniverse_SubgenreInput);

    mUniverse_AvatarTypeInput = new QLineEdit(QString::number(avatarType));
    mUniverse_AvatarTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_AvatarTypeInput));
    mContentLayout->addRow("Avatar Type", mUniverse_AvatarTypeInput);

    mUniverse_AccessTypeInput = new QLineEdit(QString::number(accessType));
    mUniverse_AccessTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_AccessTypeInput));
    mContentLayout->addRow("Access Type", mUniverse_AccessTypeInput);

    mUniverse_PaymentTypeInput = new QLineEdit(QString::number(paymentType));
    mUniverse_PaymentTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_PaymentTypeInput));
    mContentLayout->addRow("Payment Type", mUniverse_PaymentTypeInput);

    mUniverse_AgeRatingInput = new QLineEdit(QString::number(ageRating));
    mUniverse_AgeRatingInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_AgeRatingInput));
    mContentLayout->addRow("Age Rating", mUniverse_AgeRatingInput);

    mUniverse_AllowPrivateServersInput = new QCheckBox();
    mUniverse_AllowPrivateServersInput->setChecked(allowPrivateServers);
    mContentLayout->addRow("Allow Private Servers", mUniverse_AllowPrivateServersInput);

    mUniverse_AllowDirectAccessInput = new QCheckBox();
    mUniverse_AllowDirectAccessInput->setChecked(allowDirectAccess);
    mContentLayout->addRow("Allow Direct Access", mUniverse_AllowDirectAccessInput);

    mUniverse_SupportedDevicesInput = new QLineEdit(QString::fromStdString(supportedDevices));
    mUniverse_SupportedDevicesInput->setPlaceholderText("e.g. Computer,Phone,Tablet,Console");
    mContentLayout->addRow("Supported Devices", mUniverse_SupportedDevicesInput);

    // A single social link (the UniverseSocialLink table is keyed by universe Id).
    int64_t socialLinkType = 0;
    std::string socialLinkUrl;
    std::string socialLinkTitle;
    if (mId.has_value()) {
        Statement linkStmt = db->PrepareStatement("SELECT LinkType, Url, Title FROM UniverseSocialLink WHERE Id = ?");
        linkStmt.Bind(1, mId.value());
        if (linkStmt.Step() == SQLITE_ROW) {
            socialLinkType = linkStmt.GetInt64FromColumnIndex(0);
            socialLinkUrl = linkStmt.GetStringFromColumnIndex(1);
            socialLinkTitle = linkStmt.GetStringFromColumnIndex(2);
        }
    }

    AddSectionHeader("Social Link");

    mUniverse_SocialLinkTypeInput = new QLineEdit(QString::number(socialLinkType));
    mUniverse_SocialLinkTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mUniverse_SocialLinkTypeInput));
    mContentLayout->addRow("Link Type", mUniverse_SocialLinkTypeInput);

    mUniverse_SocialLinkUrlInput = new QLineEdit(QString::fromStdString(socialLinkUrl));
    mUniverse_SocialLinkUrlInput->setPlaceholderText("https://...");
    mContentLayout->addRow("Link Url", mUniverse_SocialLinkUrlInput);

    mUniverse_SocialLinkTitleInput = new QLineEdit(QString::fromStdString(socialLinkTitle));
    mUniverse_SocialLinkTitleInput->setPlaceholderText("Title shown to players");
    mContentLayout->addRow("Link Title", mUniverse_SocialLinkTitleInput);

    AddSectionHeader("Places");

    mUniverse_PlaceFrame = new QFrame();
    auto *placeFrameLayout = new QVBoxLayout(mUniverse_PlaceFrame);

    mUniverse_PlaceList = new ItemListWidget(nullptr, GetDatabase());
    mUniverse_PlaceList->setUniformItemSizes(false); // So "(Start Place)" text can show without being truncated
    mUniverse_PlaceList->SetOnContextMenuShown([this](QMenu* menu, ItemWidget* item) {
        QAction* startPlaceAction = menu->addAction(QIcon(":/images/spawn_16x16.png"), "Set as Start Place");
        QAction* removeAction = menu->addAction(QIcon(":/images/silk/cross.png"), "Remove From List");

        connect(startPlaceAction, &QAction::triggered, [this, item]() {
            for (int i = 0; i < mUniverse_PlaceList->count(); i++) {
                QListWidgetItem* other = mUniverse_PlaceList->item(i);
                QString text = other->text();
                if (text.endsWith("\n(Start Place)")) {
                    other->setText(text.chopped(QString("\n(Start Place)").length()));
                }
            }

            item->setText(item->text() + "\n(Start Place)");
            mUniverse_StartPlaceId = item->GetId();
        });

        connect(removeAction, &QAction::triggered, [this, item]() {
            int64_t placeId = item->GetId();
            mUniverse_PlaceList->Remove(item->GetType(), placeId);
            mUniverse_PendingDeletePlaces.push_back(placeId);
            // Also remove from pending adds if it was just added this session
            mUniverse_PendingPlaces.removeAll(placeId);
        });
    });

    if (mId.has_value()) {
        Statement placesStmt = db->PrepareStatement(
            "SELECT PlaceId FROM UniversePlace WHERE Id = ?");
        placesStmt.Bind(1, mId.value());
        while (placesStmt.Step() == SQLITE_ROW) {
            int64_t placeId = placesStmt.GetInt64FromColumnIndex(0);
            if (!mUniverse_PlaceList->Add(ItemType::Asset, placeId)) {
                QMessageBox::critical(this, "Error", "Failed to load place into list!");
            }
            if (placeId == startPlaceId) {
                // Get the widget just added and append the label
                ItemWidget* w = mUniverse_PlaceList->GetItemWidget(ItemType::Asset, placeId);
                if (w) w->setText(w->text() + "\n(Start Place)");
            }
        }
    }

    mUniverse_AddPlaceButton = new QPushButton("Add Place");
    placeFrameLayout->addWidget(mUniverse_PlaceList);
    placeFrameLayout->addWidget(mUniverse_AddPlaceButton);

    connect(mUniverse_AddPlaceButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Place, true);
        if (id.has_value()) {
            if (mUniverse_PlaceList->IsItemInList(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Place Already Exists", "You already added this place to the list.");
                return;
            }
            if (!mUniverse_PlaceList->Add(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Error", "Failed to add place");
                return;
            }
            mUniverse_PendingPlaces.push_back(id.value());
        }
    });

    placeFrameLayout->setContentsMargins(0, 0, 0, 0);
    mContentLayout->addRow(mUniverse_PlaceFrame);

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
    std::optional<int64_t> startPlaceId = mUniverse_StartPlaceId;
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
    stmt.Bind(3, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(4, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    if (startPlaceId.has_value())
        stmt.Bind(5, startPlaceId.value());
    else
        stmt.Bind(5); // bind as null
    stmt.Bind(6, userId);
    stmt.Bind(7, groupId);
    stmt.Bind(8, active);
    stmt.Bind(9, visits);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    int64_t favorites = mUniverse_FavoritesInput->text().toLongLong();
    int64_t likes = mUniverse_LikesInput->text().toLongLong();
    int64_t dislikes = mUniverse_DislikesInput->text().toLongLong();

    Statement histStmt = db->PrepareStatement(R"(
        INSERT INTO UniverseHistorical (Id, Favorites, Likes, Dislikes) VALUES (?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            Favorites = excluded.Favorites,
            Likes = excluded.Likes,
            Dislikes = excluded.Dislikes;
    )");
    histStmt.Bind(1, id);
    histStmt.Bind(2, favorites);
    histStmt.Bind(3, likes);
    histStmt.Bind(4, dislikes);
    if (histStmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    // Persist the universe settings (UniverseMisc table).
    int64_t genre = mUniverse_GenreInput->text().toLongLong();
    int64_t subgenre = mUniverse_SubgenreInput->text().toLongLong();
    int64_t avatarType = mUniverse_AvatarTypeInput->text().toLongLong();
    int64_t accessType = mUniverse_AccessTypeInput->text().toLongLong();
    int64_t paymentType = mUniverse_PaymentTypeInput->text().toLongLong();
    int64_t ageRating = mUniverse_AgeRatingInput->text().toLongLong();
    bool allowPrivateServers = mUniverse_AllowPrivateServersInput->isChecked();
    bool allowDirectAccess = mUniverse_AllowDirectAccessInput->isChecked();
    std::string supportedDevices = mUniverse_SupportedDevicesInput->text().toStdString();

    Statement miscStmt = db->PrepareStatement(R"(
        INSERT INTO UniverseMisc (Id, Genre, Subgenre, AvatarType, AccessType, PaymentType, AllowPrivateServers, AllowDirectAccessToPlaces, AgeRating, SupportedDevices) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            Genre = excluded.Genre,
            Subgenre = excluded.Subgenre,
            AvatarType = excluded.AvatarType,
            AccessType = excluded.AccessType,
            PaymentType = excluded.PaymentType,
            AllowPrivateServers = excluded.AllowPrivateServers,
            AllowDirectAccessToPlaces = excluded.AllowDirectAccessToPlaces,
            AgeRating = excluded.AgeRating,
            SupportedDevices = excluded.SupportedDevices;
    )");
    miscStmt.Bind(1, id);
    miscStmt.Bind(2, genre);
    miscStmt.Bind(3, subgenre);
    miscStmt.Bind(4, avatarType);
    miscStmt.Bind(5, accessType);
    miscStmt.Bind(6, paymentType);
    miscStmt.Bind(7, allowPrivateServers);
    miscStmt.Bind(8, allowDirectAccess);
    miscStmt.Bind(9, ageRating);
    miscStmt.Bind(10, supportedDevices);
    if (miscStmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    // Persist the social link. If the title and url are both blank, treat it as "no link".
    std::string socialLinkUrl = mUniverse_SocialLinkUrlInput->text().toStdString();
    std::string socialLinkTitle = mUniverse_SocialLinkTitleInput->text().toStdString();
    if (socialLinkUrl.empty() && socialLinkTitle.empty()) {
        Statement delLink = db->PrepareStatement("DELETE FROM UniverseSocialLink WHERE Id = ?;");
        delLink.Bind(1, id);
        if (delLink.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    } else {
        int64_t socialLinkType = mUniverse_SocialLinkTypeInput->text().toLongLong();
        Statement linkStmt = db->PrepareStatement(R"(
            INSERT INTO UniverseSocialLink (Id, LinkType, Url, Title) VALUES (?, ?, ?, ?)
            ON CONFLICT (Id) DO UPDATE SET
                LinkType = excluded.LinkType,
                Url = excluded.Url,
                Title = excluded.Title;
        )");
        linkStmt.Bind(1, id);
        linkStmt.Bind(2, socialLinkType);
        linkStmt.Bind(3, socialLinkUrl);
        linkStmt.Bind(4, socialLinkTitle);
        if (linkStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    // Delete removed places
    for (int64_t placeId : mUniverse_PendingDeletePlaces) {
        Statement del = db->PrepareStatement(
            "DELETE FROM UniversePlace WHERE Id = ? AND PlaceId = ?");
        del.Bind(1, id);
        del.Bind(2, placeId);
        if (del.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    // Insert new places
    for (int64_t placeId : mUniverse_PendingPlaces) {
        Statement ins = db->PrepareStatement(
            "INSERT OR IGNORE INTO UniversePlace (Id, PlaceId) VALUES (?, ?)");
        ins.Bind(1, id);
        ins.Bind(2, placeId);
        if (ins.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}
