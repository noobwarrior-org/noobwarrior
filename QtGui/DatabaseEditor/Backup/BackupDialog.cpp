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

#include <cassert>

using namespace NoobWarrior;
using namespace NoobWarrior::Backup;

BackupDialog::BackupDialog(QWidget *parent) : QDialog(parent),
    mChoseItemSource(false),
    mSource(ItemSource::OnlineItem),
    mItemType(OnlineItemType::Universe),
    mFrameLayout(nullptr)
{
    setWindowTitle(tr("Backup from Roblox"));
    setWindowIcon(QIcon(":/images/roblox_backup.png"));
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    InitWidgets();
}

void BackupDialog::InitWidgets() {
    mMainLayout = new QVBoxLayout(this);
    mItemSourceRowLayout = new QHBoxLayout();
    
    mItemSourceButtonGroup = new QButtonGroup(this);

    QLabel *itemSourceCaption = new QLabel("Is this an online item from the Roblox website, or is this a local file (.rbxm/.rbxl) on your computer?");
    QRadioButton *onlineItem = new QRadioButton("Online Item");
    onlineItem->setObjectName("OnlineItem");

    QRadioButton *localFile = new QRadioButton("Local File");
    localFile->setObjectName("LocalFile");

    mItemSourceButtonGroup->addButton(onlineItem);
    mItemSourceButtonGroup->addButton(localFile);

    mItemSourceRowLayout->addWidget(onlineItem);
    mItemSourceRowLayout->addWidget(localFile);

    connect(mItemSourceButtonGroup, &QButtonGroup::buttonToggled, [this](QAbstractButton *button, bool checked) {
        mChoseItemSource = true;
        mSource = button->objectName().contains("OnlineItem") ? ItemSource::OnlineItem : ItemSource::LocalFile;
        UpdateWidgets();
    });

    mItemTypeCaption = new QLabel("Select which type of item you'd like to back up.\nIt is important to understand that a Place and a Universe are not the same thing.\nUniverses are the entire game, while places are just individual levels.\nIt is also important to know that an Asset can be one of many types, like audios or decals.");

    mItemTypeDropdown = new QComboBox();
    mItemTypeDropdown->addItem("Universe (Game)", static_cast<int>(OnlineItemType::Universe));
    mItemTypeDropdown->addItem("Model/Place/Asset", static_cast<int>(OnlineItemType::Asset));
    mItemTypeDropdown->addItem("User", static_cast<int>(OnlineItemType::User));
    mItemTypeDropdown->addItem("Group", static_cast<int>(OnlineItemType::Group));
    mItemTypeDropdown->addItem("Bundle", static_cast<int>(OnlineItemType::Bundle));

    connect(mItemTypeDropdown, &QComboBox::currentIndexChanged, [this](int index) {
        mItemType = static_cast<OnlineItemType>(mItemTypeDropdown->itemData(index).toInt());
        UpdateWidgets();
    });

    mIdCaption = new QLabel("Type in the ID of the item. Make sure you chose the correct type.");
    mIdField = new QLineEdit();
    mIdField->setPlaceholderText("Item ID");

    mFrame = new QFrame();
    mFrame->setFrameStyle(QFrame::Panel);
    mFrameLayout = new QVBoxLayout(mFrame);

    mButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtons, &QDialogButtonBox::accepted, [this]() {
        StartBackup();
        close();
    });
    connect(mButtons, &QDialogButtonBox::rejected, [this]() {
        close();
    });

    mMainLayout->addWidget(itemSourceCaption);
    mMainLayout->addLayout(mItemSourceRowLayout);
    mMainLayout->addWidget(mItemTypeCaption);
    mMainLayout->addWidget(mItemTypeDropdown);
    mMainLayout->addWidget(mIdCaption);
    mMainLayout->addWidget(mIdField);
    mMainLayout->addWidget(mFrame);
    mMainLayout->addWidget(mButtons);

    // defaults
    // onlineItem->toggle();
    // mItemTypeUniverse->toggle();
    UpdateWidgets();
}

void BackupDialog::UpdateWidgets() {
    assert(mFrameLayout != nullptr && "mFrameLayout cannot be null");
    qDeleteAll(mFrame->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));

    // initially hide all of these at first since we want each of these to individually pop up one by one as the user flows through the dialog
    mItemTypeCaption->setVisible(mChoseItemSource && mSource == ItemSource::OnlineItem);
    mItemTypeDropdown->setVisible(mChoseItemSource && mSource == ItemSource::OnlineItem);

    mIdCaption->setVisible(mChoseItemSource && mSource == ItemSource::OnlineItem);
    mIdField->setVisible(mChoseItemSource && mSource == ItemSource::OnlineItem);

    mFrame->setVisible(mChoseItemSource || mSource == ItemSource::LocalFile);

    if (mSource == ItemSource::OnlineItem) {
        if (mItemType == OnlineItemType::Universe) {
            
        } else if (mItemType == OnlineItemType::Asset) {

        } else if (mItemType == OnlineItemType::User) {

        }
    } else if (mSource == ItemSource::LocalFile) {
        auto* fileAddressLayout = new QHBoxLayout();
        auto* fileAddressInput = new QLineEdit();
        auto* fileBrowseButton = new QPushButton(QIcon(":/images/silk/folder_magnify.png"), "Browse");

        connect(fileBrowseButton, &QPushButton::clicked, [this, fileAddressInput]() {
            QString filePath = QFileDialog::getOpenFileName(this, tr("Select Place/Model File"), QString(), tr("Roblox File Format (*.rbxl *.rbxlx *.rbxm *.rbxmx)"));
            fileAddressInput->setText(filePath);
        });

        fileAddressInput->setPlaceholderText("Place/Model File Path");

        fileAddressLayout->addWidget(fileAddressInput);
        fileAddressLayout->addWidget(fileBrowseButton);

        mFrameLayout->addLayout(fileAddressLayout);
    }

    resize(minimumSizeHint());
}

void BackupDialog::InitOnlineUniverseWidgets() {

}

void BackupDialog::InitOnlineAssetWidgets() {

}

void BackupDialog::InitLocalFileWidgets() {

}

void BackupDialog::StartBackup() {

}