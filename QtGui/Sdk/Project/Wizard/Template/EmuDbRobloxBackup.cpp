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
// File: EmuDbRobloxBackup.cpp
// Started by: Hattozo
// Started on: 2/2/2026
// Description:
#include "EmuDbRobloxBackup.h"
#include "TemplatePage.h"
#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/Backup/BackupTask.h"
#include "Application.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Backup.h>
#include <NoobWarrior/Keychain/Keychain.h>
#include <NoobWarrior/Log.h>

#include <filesystem>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpressionValidator>

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

// itemData sentinel in the type dropdown for the "back up a local file" mode; real entries carry
// a non-negative ItemType.
static constexpr int kLocalFileEntry = -1;

EmuDbRobloxBackupIntroPage::EmuDbRobloxBackupIntroPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("Roblox Backup");
    setSubTitle("Choose the Roblox item to back up and where to store it. The backup runs in the background once you finish.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("My Cool Backup");
    mTitleEdit->setClearButtonEnabled(true);
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);

    mPathEdit = new QLineEdit();
    mPathEdit->setText(QString::fromStdString((gApp->GetCore()->GetUserDataDir() / "databases").string()));
    mFormLayout->addRow(new QLabel("File Path"), mPathEdit);

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
    mFormLayout->addRow(new QLabel("Item Type"), mItemTypeDropdown);

    mItemIdEdit = new QLineEdit();
    mItemIdEdit->setPlaceholderText("Item ID (e.g. a universe / game ID)");
    mItemIdValidator = new QRegularExpressionValidator(QRegularExpression("[0-9]*"), this);
    mItemIdEdit->setValidator(mItemIdValidator);

    mBrowseButton = new QPushButton("Browse...");
    mBrowseButton->setVisible(false);
    connect(mBrowseButton, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Choose a Roblox file", QString(),
            "Roblox files (*.rbxl *.rbxlx *.rbxm *.rbxmx);;All files (*.*)");
        if (!path.isEmpty())
            mItemIdEdit->setText(path);
    });

    auto* idRow = new QHBoxLayout();
    idRow->addWidget(mItemIdEdit);
    idRow->addWidget(mBrowseButton);
    mItemIdLabel = new QLabel("Item ID");
    mFormLayout->addRow(mItemIdLabel, idRow);

    // The options mirror Backup::ProcessOptions; the defaults match what the wizard always did.
    mMetadataCheck = new QCheckBox("Download metadata (names, descriptions, creators)");
    mMetadataCheck->setChecked(true);
    mThumbnailsCheck = new QCheckBox("Download rendered thumbnails");
    mThumbnailsCheck->setChecked(true);
    mParseFilesCheck = new QCheckBox("Scan downloaded files and back up the assets they reference");
    mParseFilesCheck->setChecked(true);
    mUseItemMetaCheck = new QCheckBox("Use the item's name and icon for this database");
    mUseItemMetaCheck->setChecked(true);
    mMainLayout->addWidget(mMetadataCheck);
    mMainLayout->addWidget(mThumbnailsCheck);
    mMainLayout->addWidget(mParseFilesCheck);
    mMainLayout->addWidget(mUseItemMetaCheck);

    connect(mItemTypeDropdown, &QComboBox::currentIndexChanged, [this](int index) {
        bool localFile = mItemTypeDropdown->itemData(index).toInt() == kLocalFileEntry;
        mItemIdEdit->clear();
        mItemIdLabel->setText(localFile ? "File Path" : "Item ID");
        mItemIdEdit->setPlaceholderText(localFile ? "Path to a .rbxl / .rbxm file. Every asset it references is backed up."
                                                  : "Item ID (e.g. a universe / game ID)");
        mItemIdEdit->setValidator(localFile ? nullptr : mItemIdValidator);
        mBrowseButton->setVisible(localFile);
        // A local file has no online metadata to stamp the database with.
        mUseItemMetaCheck->setEnabled(!localFile);
        completeChanged();
    });

    mLoginNoticeLabel = new QLabel();
    mLoginNoticeLabel->setWordWrap(true);
    mMainLayout->addWidget(mLoginNoticeLabel);
    if (gApp->GetCore()->GetRbxKeychain()->IsLoggedIn()) {
        mLoginNoticeLabel->setVisible(false);
    } else {
        mLoginNoticeLabel->setText("Note: you are not logged into a Roblox account. Public information (names, places, thumbnails) is still scraped, but asset files that require authentication will be skipped. Log in for a complete backup.");
    }

    // The title drives the database filename; characters Windows forbids (and path separators,
    // so a title can't traverse directories) are dropped.
    connect(mTitleEdit, &QLineEdit::textChanged, [this]() {
        std::filesystem::path path(gApp->GetCore()->GetUserDataDir() / "databases");
        QString fileName = mTitleEdit->text().toLower().replace(" ", "_");
        fileName.remove(QRegularExpression("[\\\\/:*?\"<>|]"));
        if (fileName.isEmpty())
            fileName = "backup";
        fileName += ".nwdb";
        path /= fileName.toStdString();
        mPathEdit->setText(QString::fromStdString(path.string()));
        completeChanged();
    });
    connect(mPathEdit, &QLineEdit::textChanged, this, &EmuDbRobloxBackupIntroPage::completeChanged);
    connect(mItemIdEdit, &QLineEdit::textChanged, this, &EmuDbRobloxBackupIntroPage::completeChanged);
}

bool EmuDbRobloxBackupIntroPage::validatePage() {
    if (!isComplete())
        return false;
    if (!TemplatePage::validatePage())
        return false;

    Sdk* sdk = dynamic_cast<Sdk*>(wizard()->parent());
    if (sdk == nullptr) {
        gApp->GetCore()->Out("EmuDbRobloxBackupIntroPage", "Failed to create project: Sdk is not a parent of wizard");
        return false;
    }

    auto* project = new EmuDbProject(mPathEdit->text().toStdString());
    if (project->Fail()) {
        QMessageBox::critical(this, "Cannot Create Project",
            QString("Failed to create the project.\nMessage received: \"%1\"").arg(project->GetFailMsg()));
        delete project;
        return false;
    }
    
    project->GetDb()->SetTitle(mTitleEdit->text().toStdString());
    sdk->AddProject(project);

    const bool localFile = mItemTypeDropdown->currentData().toInt() == kLocalFileEntry;

    ProcessOptions opts;
    opts.TargetSource = localFile ? ItemSource::LocalFile : ItemSource::OnlineItem;
    if (localFile) {
        opts.TargetFilePath = mItemIdEdit->text().trimmed().toStdString();
        opts.TargetItemType = ItemType::Asset;
    } else {
        opts.TargetItemType = static_cast<ItemType>(mItemTypeDropdown->currentData().toInt());
        opts.TargetId = mItemIdEdit->text().toLongLong();
    }
    opts.DestinationType = DestinationType::Database;
    opts.Destination = project->GetDb();
    opts.DownloadMetadata = mMetadataCheck->isChecked();
    opts.DownloadAutoGeneratedThumbnails = mThumbnailsCheck->isChecked();
    opts.ParseFilesAndBackupFoundAssets = mParseFilesCheck->isChecked();
    // Stamping would overwrite the user's chosen title with the file name.
    opts.SetDestinationMetaFromTarget = !localFile && mUseItemMetaCheck->isChecked();

    auto* task = new BackupTask(gApp->GetCore(), std::move(opts), sdk);
    task->SetProject(project);
    task->Start();

    return true;
}

bool EmuDbRobloxBackupIntroPage::isComplete() const {
    if (mTitleEdit->text().isEmpty() || mPathEdit->text().isEmpty())
        return false;
    // Don't clobber an existing file.
    if (std::filesystem::exists(mPathEdit->text().toStdString()))
        return false;
    if (mItemTypeDropdown->currentData().toInt() == kLocalFileEntry) {
        std::string sourcePath = mItemIdEdit->text().trimmed().toStdString();
        std::error_code ec; // the throwing overload can throw on malformed paths mid-typing
        return !sourcePath.empty() && std::filesystem::is_regular_file(sourcePath, ec);
    }
    bool idOk = false;
    qlonglong id = mItemIdEdit->text().toLongLong(&idOk);
    return idOk && id > 0;
}

int EmuDbRobloxBackupIntroPage::nextId() const {
    return -1;
}

QString EmuDbRobloxBackupIntroPage::GetName() {
    return "Roblox Backup";
}

QString EmuDbRobloxBackupIntroPage::GetDescription() {
    return "This template backs up a Roblox item. It can be almost any item available on the website (or a local file as they can reference assets.) Any item that is found is scraped into a database.\n\nLogging into a Roblox account is required for this to work properly, as Roblox has gated their asset delivery mechanism and many other API endpoints behind authentication.";
}

QIcon EmuDbRobloxBackupIntroPage::GetIcon() {
    return QIcon(":/images/db_backupgame_96x96.png");
}
