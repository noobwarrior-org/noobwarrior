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
// File: BackupDialog.cpp
// Started by: Hattozo
// Started on: 11/3/2025
// Description:
#include "BackupDialog.h"
#include "Application.h"
#include "BackupTask.h"
#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "NoobWarrior/Backup.h"

#include <cassert>
#include <filesystem>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpressionValidator>

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

// Real entries carry a non-negative ItemType.
static constexpr int kLocalFileEntry = -1;

BackupDialog::BackupDialog(QWidget *parent) : QDialog(parent),
    mSource(ItemSource::OnlineItem),
    mItemType(ItemType::Universe)
{
    setWindowTitle(tr("Backup from Roblox"));
    setWindowIcon(QIcon(":/images/roblox_backup.png"));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    InitWidgets();
}

void BackupDialog::InitWidgets() {
    mMainLayout = new QVBoxLayout(this);

    mItemTypeCaption = new QLabel("Select which type of item you'd like to back up.\nIt is important to understand that a Place and a Universe are not the same thing.\nUniverses are the entire game, while places are just individual levels.\nIt is also important to know that an Asset can be one of many types, like audios or decals.");

    mItemTypeDropdown = new QComboBox();
    mItemTypeDropdown->addItem("Universe (Game)", static_cast<int>(ItemType::Universe));
    mItemTypeDropdown->addItem("Model/Place/Asset", static_cast<int>(ItemType::Asset));
    mItemTypeDropdown->addItem("User", static_cast<int>(ItemType::User));
    mItemTypeDropdown->addItem("Group", static_cast<int>(ItemType::Group));
    mItemTypeDropdown->addItem("Bundle", static_cast<int>(ItemType::Bundle));
    mItemTypeDropdown->addItem("Badge", static_cast<int>(ItemType::Badge));
    mItemTypeDropdown->addItem("Game Pass", static_cast<int>(ItemType::Pass));
    mItemTypeDropdown->addItem("Developer Product", static_cast<int>(ItemType::DevProduct));
    mItemTypeDropdown->addItem("Outfit", static_cast<int>(ItemType::Outfit));
    mItemTypeDropdown->addItem("Local File (.rbxl / .rbxm)", kLocalFileEntry);

    connect(mItemTypeDropdown, &QComboBox::currentIndexChanged, [this](int index) {
        int data = mItemTypeDropdown->itemData(index).toInt();
        mIdField->clear();
        if (data == kLocalFileEntry) {
            mSource = ItemSource::LocalFile;
            mIdCaption->setText("Choose the place or model file to scan.\nEvery asset it references (meshes, textures, decals, sounds, rbxassetid:// links in scripts, ...) is backed up.");
            mIdField->setPlaceholderText("Path to .rbxl / .rbxm file");
            mIdField->setValidator(nullptr);
        } else {
            mSource = ItemSource::OnlineItem;
            mItemType = static_cast<ItemType>(data);
            mIdCaption->setText("Type in the ID of the item. Make sure you chose the correct type.");
            mIdField->setPlaceholderText("Item ID");
            mIdField->setValidator(mIdValidator);
        }
        mBrowseButton->setVisible(data == kLocalFileEntry);
        UpdateWidgets();
    });

    mIdCaption = new QLabel("Type in the ID of the item. Make sure you chose the correct type.");
    mIdField = new QLineEdit();
    mIdField->setPlaceholderText("Item ID");
    mIdValidator = new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this);
    mIdField->setValidator(mIdValidator);

    mBrowseButton = new QPushButton("Browse...");
    mBrowseButton->setVisible(false);
    connect(mBrowseButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Choose a Roblox file", QString(),
            "Roblox files (*.rbxl *.rbxlx *.rbxm *.rbxmx);;All files (*.*)");
        if (!path.isEmpty())
            mIdField->setText(path);
    });

    mButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtons, &QDialogButtonBox::accepted, [this]() {
        if (StartBackup())
            accept();
    });
    connect(mButtons, &QDialogButtonBox::rejected, [this]() {
        reject();
    });
    
    mMetadataCheck = new QCheckBox("Download metadata (names, descriptions, creators)");
    mMetadataCheck->setChecked(true);
    mThumbnailsCheck = new QCheckBox("Download rendered thumbnails");
    mThumbnailsCheck->setChecked(true);
    mParseFilesCheck = new QCheckBox("Scan downloaded files and back up the assets they reference");
    mParseFilesCheck->setChecked(true);

    mMainLayout->addWidget(mItemTypeCaption);
    mMainLayout->addWidget(mItemTypeDropdown);
    mMainLayout->addWidget(mIdCaption);
    auto* idRow = new QHBoxLayout();
    idRow->addWidget(mIdField);
    idRow->addWidget(mBrowseButton);
    mMainLayout->addLayout(idRow);
    mMainLayout->addWidget(mMetadataCheck);
    mMainLayout->addWidget(mThumbnailsCheck);
    mMainLayout->addWidget(mParseFilesCheck);
    mMainLayout->addWidget(mButtons);

    UpdateWidgets();
}

void BackupDialog::UpdateWidgets() {
    resize(minimumSizeHint());
}

bool BackupDialog::StartBackup() {
    EmuDb *db = GetDatabase();
    if (db == nullptr) {
        QMessageBox::critical(this, "Cannot Backup", "Your SDK window needs to be tabbed into a database project.");
        return false;
    }

    ProcessOptions opts;
    opts.TargetSource = mSource;
    if (mSource == ItemSource::LocalFile) {
        std::string path = mIdField->text().trimmed().toStdString();
        std::error_code ec; // the throwing overload can throw on malformed paths
        if (path.empty() || !std::filesystem::is_regular_file(path, ec)) {
            QMessageBox::warning(this, "Invalid File", "Please choose an existing Roblox place or model file.");
            return false;
        }
        opts.TargetFilePath = path;
        opts.TargetItemType = ItemType::Asset;
        gApp->GetCore()->Out("BackupDialog", "Started backup of local file {}", path);
    } else {
        bool idOk = false;
        int64_t id = mIdField->text().trimmed().toLongLong(&idOk);
        if (!idOk || id <= 0) {
            QMessageBox::warning(this, "Invalid ID", "Please enter a valid numeric item ID.");
            return false;
        }
        opts.TargetItemType = mItemType;
        opts.TargetId = id;
        gApp->GetCore()->Out("BackupDialog", "Started backup of {} id {}", GetTableNameFromItemType(mItemType), id);
    }
    opts.DestinationType = DestinationType::Database;
    opts.Destination = db;
    opts.DownloadMetadata = mMetadataCheck->isChecked();
    opts.DownloadAutoGeneratedThumbnails = mThumbnailsCheck->isChecked();
    opts.ParseFilesAndBackupFoundAssets = mParseFilesCheck->isChecked();

    auto *sdk = dynamic_cast<Sdk*>(parent());
    if (sdk == nullptr) {
        QMessageBox::critical(this, "Cannot Backup", "The backup window has no parent SDK window to run the task in.");
        return false;
    }

    // The task parents itself to the SDK window and deletes itself when the run finishes.
    auto *task = new BackupTask(gApp->GetCore(), std::move(opts), sdk);
    task->SetProject(sdk->GetFocusedProject());
    task->Start();
    return true;
}

EmuDb* BackupDialog::GetDatabase() {
    auto *sdk = dynamic_cast<Sdk*>(parent());
    if (sdk != nullptr) {
        auto* proj = dynamic_cast<EmuDbProject*>(sdk->GetFocusedProject());
        if (proj != nullptr) return proj->GetDb();
    }
    return nullptr;
}