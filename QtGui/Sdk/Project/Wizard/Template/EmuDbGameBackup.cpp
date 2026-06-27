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
// File: EmuDbGameBackup.cpp
// Started by: Hattozo
// Started on: 2/2/2026
// Description:
#include "EmuDbGameBackup.h"
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

#include <QMessageBox>
#include <QRegularExpressionValidator>

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

EmuDbGameBackupIntroPage::EmuDbGameBackupIntroPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("Roblox Game Backup");
    setSubTitle("Choose the Roblox item to back up and where to store it. The backup runs in the background once you finish.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("My Cool Game");
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
    mFormLayout->addRow(new QLabel("Item Type"), mItemTypeDropdown);

    mItemIdEdit = new QLineEdit();
    mItemIdEdit->setPlaceholderText("Item ID (e.g. a universe / game ID)");
    mItemIdEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mItemIdEdit));
    mFormLayout->addRow(new QLabel("Item ID"), mItemIdEdit);

    mLoginNoticeLabel = new QLabel();
    mLoginNoticeLabel->setWordWrap(true);
    mMainLayout->addWidget(mLoginNoticeLabel);
    if (gApp->GetCore()->GetRbxKeychain()->IsLoggedIn()) {
        mLoginNoticeLabel->setVisible(false);
    } else {
        mLoginNoticeLabel->setText("Note: you are not logged into a Roblox account. Public information (names, places, thumbnails) is still scraped, but asset files that require authentication will be skipped. Log in for a complete backup.");
    }

    // The title drives the database filename, mirroring the empty-database template.
    connect(mTitleEdit, &QLineEdit::textChanged, [this]() {
        std::filesystem::path path(gApp->GetCore()->GetUserDataDir() / "databases");
        QString fileName = mTitleEdit->text().toLower().replace(" ", "_") + ".nwdb";
        path /= fileName.toStdString();
        mPathEdit->setText(QString::fromStdString(path.string()));
        completeChanged();
    });
    connect(mPathEdit, &QLineEdit::textChanged, this, &EmuDbGameBackupIntroPage::completeChanged);
    connect(mItemIdEdit, &QLineEdit::textChanged, this, &EmuDbGameBackupIntroPage::completeChanged);
}

bool EmuDbGameBackupIntroPage::validatePage() {
    if (!isComplete())
        return false;
    if (!TemplatePage::validatePage())
        return false;

    Sdk* sdk = dynamic_cast<Sdk*>(wizard()->parent());
    if (sdk == nullptr) {
        Out("EmuDbGameBackupIntroPage", "Failed to create project: Sdk is not a parent of wizard");
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

    ProcessOptions opts;
    opts.TargetSource = ItemSource::OnlineItem;
    opts.TargetItemType = static_cast<ItemType>(mItemTypeDropdown->currentData().toInt());
    opts.TargetId = mItemIdEdit->text().toLongLong();
    opts.DestinationType = DestinationType::Database;
    opts.Destination = project->GetDb();
    opts.DownloadMetadata = true;
    opts.DownloadAutoGeneratedThumbnails = true;
    opts.ParseFilesAndBackupFoundAssets = false;
    opts.SetDestinationMetaFromTarget = true;

    auto* task = new BackupTask(gApp->GetCore(), std::move(opts));
    task->SetSdk(sdk);
    task->SetProject(project);
    task->Start();

    return true;
}

bool EmuDbGameBackupIntroPage::isComplete() const {
    if (mTitleEdit->text().isEmpty() || mPathEdit->text().isEmpty())
        return false;
    // Don't clobber an existing file.
    if (std::filesystem::exists(mPathEdit->text().toStdString()))
        return false;
    bool idOk = false;
    qlonglong id = mItemIdEdit->text().toLongLong(&idOk);
    return idOk && id > 0;
}

int EmuDbGameBackupIntroPage::nextId() const {
    return -1;
}

QString EmuDbGameBackupIntroPage::GetName() {
    return "Game Backup";
}

QString EmuDbGameBackupIntroPage::GetDescription() {
    return "This template backs up your game, either online or local, and scrapes whatever it can into a database.\n\nIf you are attempting to download assets or other data, logging into a Roblox account is required. Please use a burner account if possible.";
}

QIcon EmuDbGameBackupIntroPage::GetIcon() {
    return QIcon(":/images/db_backupgame_96x96.png");
}
