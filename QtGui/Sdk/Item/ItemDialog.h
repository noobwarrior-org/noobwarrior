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
// File: ItemDialog.h
// Started by: Hattozo
// Started on: 10/27/2025
// Description: Dialog window that allows you to edit or create an item.
#pragma once
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QComboBox>
#include <QTreeView>
#include <QStandardItemModel>
#include <QListWidget>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QTableWidget>
#include <QSlider>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QTemporaryFile>

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>

#include <memory>
#include <optional>
#include <fstream>

#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/CreatorInfoWidget.h"
#include "ItemListWidget.h"

namespace NoobWarrior {
class ItemDialog : public QDialog {
public:
    ItemDialog(EmuDb* db, ItemType type, std::optional<int64_t> id = std::nullopt, QWidget *parent = nullptr);
protected:
    void RegenWidgets();
    void OnSave();

    // Adds a styled, full-width section heading to the content form to visually group fields.
    void AddSectionHeader(const QString &title);

    void AddOwnedItemFields();

    // Creates an Image asset from a file on disk (uploading its data as a blob) and
    // returns the new asset's Id, or std::nullopt on failure.
    std::optional<int64_t> CreateImageAssetFromFile(const QString &filePath);

    void Asset_AddFields();
    void Asset_AddFields_AssetType();
    void Asset_AddFields_Place();
    void Asset_AddFields_MediaPreview();
    // Loads the asset's latest saved data (or the most recent pending file) into the media player so
    // Audio/Video assets can be played back. Lazy: only reads/decodes once per dialog.
    void Asset_LoadMediaPreview();
    void Asset_AddThumbnailToList(int64_t thumbnailId, bool pendingAdd);
    void Asset_SetVisibilityOfAssetTypeWidgets(Roblox::AssetType type);
    Roblox::AssetType Asset_GetAssetTypeFromFileType(const std::filesystem::path &path);
    bool Asset_OnSave();

    void Badge_AddFields();
    bool Badge_OnSave();

    void Bundle_AddFields();
    bool Bundle_OnSave();

    void DevProduct_AddFields();
    bool DevProduct_OnSave();

    void Group_AddFields();
    bool Group_OnSave();

    void Outfit_AddFields();
    bool Outfit_OnSave();

    void Pass_AddFields();
    bool Pass_OnSave();

    void Set_AddFields();
    bool Set_OnSave();

    void Universe_AddFields();
    void Universe_AddSocialLinkRow(int linkType, const QString &title, const QString &url);
    bool Universe_OnSave();

    void User_AddFields();
    bool User_OnSave();

    EmuDb* GetDatabase();

    EmuDb* mDb;
    ItemType mType;

    std::optional<int64_t> mId;

    QLabel* mIcon;

    QVBoxLayout* mRootLayout;
    QHBoxLayout* mLayout;
    QVBoxLayout* mSidebarLayout;
    QFormLayout* mContentLayout;

    QLineEdit* mIdInput;
    QLineEdit* mImageIdInput;
    QLineEdit* mNameInput;

    QPushButton* mUploadImageButton;
    QPushButton* mUseExistingImageButton;

    QPlainTextEdit* mOwned_DescriptionInput;
    QDateTimeEdit* mOwned_CreatedInput;
    QDateTimeEdit* mOwned_UpdatedInput;
    CreatorInfoWidget* mOwned_CreatorInfoWidget;

    QComboBox* mAsset_AssetCategoryInput;
    QComboBox* mAsset_AssetTypeInput;
    QTreeView* mAsset_DataView;
    QStandardItemModel* mAsset_DataModel;
    QList<QString> mAsset_DataPendingFiles;
    QList<int> mAsset_DataPendingDeleteVersions;

    QCheckBox* mAsset_PublicInput;
    QComboBox* mAsset_MinMembershipInput;
    QCheckBox* mAsset_ContentRatingInput;
    QCheckBox* mAsset_IsNewInput;
    QLineEdit* mAsset_SalesInput;
    QLineEdit* mAsset_FavoritesInput;
    QLineEdit* mAsset_LikesInput;
    QLineEdit* mAsset_DislikesInput;

    QComboBox* mAsset_SaleCurrencyInput;
    QLineEdit* mAsset_SalePriceInput;
    QComboBox* mAsset_LimitedTypeInput;
    QLineEdit* mAsset_RemainingInput;

    QGroupBox* mAsset_Place_AttributesGroup;
    QLineEdit* mAsset_Place_MaxPlayersInput;
    QCheckBox* mAsset_Place_AllowDirectAccessInput;
    QComboBox* mAsset_Place_GearGenreInput;
    QList<QCheckBox*> mAsset_Place_GearTypeChecks;

    // Audio/Video preview (shown only when the asset type is Audio or Video).
    QFrame* mAsset_MediaFrame { nullptr };
    QVideoWidget* mAsset_MediaVideoWidget { nullptr };
    QMediaPlayer* mAsset_MediaPlayer { nullptr };
    QAudioOutput* mAsset_MediaAudioOutput { nullptr };
    QPushButton* mAsset_MediaPlayButton { nullptr };
    QSlider* mAsset_MediaSeekSlider { nullptr };
    QSlider* mAsset_MediaVolumeSlider { nullptr };
    QLabel* mAsset_MediaStatusLabel { nullptr };
    // The decoded asset bytes are written here so QMediaPlayer can play them back from a real file
    // (more reliable across backends than a QIODevice source). Owned by the dialog so it outlives playback.
    QTemporaryFile* mAsset_MediaTempFile { nullptr };
    bool mAsset_MediaPreviewLoaded { false };

    QFrame* mAsset_Place_ThumbnailFrame;
    QListWidget* mAsset_Place_ThumbnailList;
    QPushButton* mAsset_Place_UploadThumbnailButton;
    QPushButton* mAsset_Place_AddThumbnailFromExistingImageButton;
    QList<int64_t> mAsset_Place_DataPendingThumbnails;
    QList<int64_t> mAsset_Place_DataPendingDeleteThumbnails;

    std::optional<int64_t> mUniverse_StartPlaceId;
    QLineEdit* mUniverse_VisitsInput;
    QCheckBox* mUniverse_ActiveInput;
    QLineEdit* mUniverse_FavoritesInput;
    QLineEdit* mUniverse_LikesInput;
    QLineEdit* mUniverse_DislikesInput;
    QComboBox* mUniverse_GenreInput;
    QComboBox* mUniverse_SubgenreInput;
    QComboBox* mUniverse_AvatarTypeInput;
    QComboBox* mUniverse_AccessTypeInput;
    QComboBox* mUniverse_PaymentTypeInput;
    QComboBox* mUniverse_AgeRatingInput;
    QCheckBox* mUniverse_AllowPrivateServersInput;
    QCheckBox* mUniverse_AllowDirectAccessInput;
    QLineEdit* mUniverse_SupportedDevicesInput;
    QCheckBox* mUniverse_VoiceChatEnabledInput;
    QTableWidget* mUniverse_SocialLinkTable;
    QPushButton* mUniverse_AddSocialLinkButton;
    QFrame* mUniverse_PlaceFrame;
    ItemListWidget* mUniverse_PlaceList;
    QPushButton* mUniverse_AddPlaceButton;
    QList<int64_t> mUniverse_PendingPlaces;
    QList<int64_t> mUniverse_PendingDeletePlaces;

    QLineEdit* mUser_DisplayNameInput;
    QLineEdit* mUser_StatusInput;
    QLineEdit* mUser_BioInput;
    QLineEdit* mUser_EmailInput;
    QDateTimeEdit* mUser_JoinDateInput;
    QDateTimeEdit* mUser_LastOnlineInput;
    QLineEdit* mUser_PlaceVisitsInput;
    QLineEdit* mUser_RankInput;
    QLineEdit* mUser_FriendCountInput;
    QLineEdit* mUser_FollowersCountInput;
    QLineEdit* mUser_FollowingCountInput;
    QLineEdit* mUser_CharacterBodyTypeInput;
    QLineEdit* mUser_CharacterWidthInput;
    QLineEdit* mUser_CharacterHeightInput;
    QLineEdit* mUser_CharacterHeadInput;
    QLineEdit* mUser_CharacterProportionsInput;
    ItemListWidget* mUser_CharacterItemList;
    QPushButton* mUser_AddCharacterItemButton;
    QList<int64_t> mUser_PendingCharacterItems;
    QList<int64_t> mUser_PendingDeleteCharacterItems;

    QCheckBox* mBadge_EnabledInput;
    QLineEdit* mBadge_UniverseIdInput;
    QLineEdit* mBadge_AwardedInput;
    QLineEdit* mBadge_AwardedYesterdayInput;

    QComboBox* mBundle_TypeInput;
    QLineEdit* mBundle_PriceInput;
    QCheckBox* mBundle_IsForSaleInput;
    QCheckBox* mBundle_IsNewInput;
    QCheckBox* mBundle_IsLimitedInput;
    QCheckBox* mBundle_IsLimitedUniqueInput;
    QLineEdit* mBundle_RemainingInput;
    QLineEdit* mBundle_SalesInput;
    QLineEdit* mBundle_FavoritesInput;
    ItemListWidget* mBundle_AssetList;
    QPushButton* mBundle_AddAssetButton;
    QList<int64_t> mBundle_PendingAssets;
    QList<int64_t> mBundle_PendingDeleteAssets;

    QComboBox* mDevProduct_CurrencyTypeInput;
    QLineEdit* mDevProduct_PriceInput;
    QLineEdit* mDevProduct_UniverseIdInput;

    QLineEdit* mGroup_OwnerIdInput;
    QLineEdit* mGroup_FundsInput;
    QLineEdit* mGroup_ShoutInput;
    QCheckBox* mGroup_EnemyDeclarationsEnabledInput;

    QLineEdit* mOutfit_UserIdInput;
    QLineEdit* mOutfit_BodyTypeInput;
    QLineEdit* mOutfit_WidthInput;
    QLineEdit* mOutfit_HeightInput;
    QLineEdit* mOutfit_HeadInput;
    QLineEdit* mOutfit_ProportionsInput;
    ItemListWidget* mOutfit_ItemList;
    QPushButton* mOutfit_AddItemButton;
    QList<int64_t> mOutfit_PendingItems;
    QList<int64_t> mOutfit_PendingDeleteItems;

    QLineEdit* mPass_UniverseIdInput;
    QLineEdit* mPass_PriceInput;
    QCheckBox* mPass_IsForSaleInput;
    QLineEdit* mPass_LikesInput;
    QLineEdit* mPass_DislikesInput;

    QLineEdit* mSet_SubscribersInput;
    ItemListWidget* mSet_AssetList;
    QPushButton* mSet_AddAssetButton;
    QList<int64_t> mSet_PendingAssets;
    QList<int64_t> mSet_PendingDeleteAssets;

    QDialogButtonBox* mButtonBox;
};
}
