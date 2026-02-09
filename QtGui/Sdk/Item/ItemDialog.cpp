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
// File: ItemDialog.cpp
// Started by: Hattozo
// Started on: 2/8/2026
// Description: Dialog window that allows you to edit or create an item.
#include "ItemDialog.h"

#define RAND_MAX 2147483647

using namespace NoobWarrior;

ItemDialog::ItemDialog(QWidget *parent, std::optional<int64_t> id, std::optional<int64_t> snapshot) :
    QDialog(parent),
    mId(id),
    mSnapshot(snapshot)
{
    assert(dynamic_cast<Sdk*>(this->parent()) != nullptr && "ItemDialog should not be parented to anything other than Sdk");
    mSdk = dynamic_cast<Sdk*>(this->parent());

    setWindowTitle("Item Editor");
}

void ItemDialog::RegenWidgets() {
    setWindowTitle(tr("Configure %1").arg(QString::fromStdString(Item::TypeName)));

    qDeleteAll(findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    mLayout = new QHBoxLayout(this);
    mSidebarLayout = new QVBoxLayout();
    mContentLayout = new QFormLayout();

    mLayout->addLayout(mSidebarLayout);
    mLayout->addLayout(mContentLayout);

    ////////////////////////////////////////////////////////////////////////
    /// icon
    ////////////////////////////////////////////////////////////////////////
    mIcon = new QLabel();
    mIcon->setAlignment(Qt::AlignLeft);
    mSidebarLayout->addWidget(mIcon);
    mSidebarLayout->addStretch();

    QImage image;

    // std::vector<unsigned char> data;

    /*
    if (mId.has_value())
        data = std::move(mDatabase->RetrieveContentImageData(mItemType, mId.has_value() ? mId.value() : -1));
    else
        data.assign(mItemType.DefaultImage, mItemType.DefaultImage + mItemType.DefaultImageSize);
    */

    // image.loadFromData(data);

    QPixmap pixmap = QPixmap::fromImage(image);
    mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (!(Item::TypeName.compare("Asset") == 0 || Item::TypeName.compare("User") == 0)) {
        auto *changeIcon = new QPushButton("Change Icon");
        mSidebarLayout->addWidget(changeIcon);
        connect(changeIcon, &QPushButton::clicked, [this]() {
            QString filePath = QFileDialog::getOpenFileName(
                this,
                "Change Icon",
                QString(),
                "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
            );
            if (!filePath.isEmpty()) {
                std::ifstream file(filePath.toStdString());

                if (!file.is_open()) {
                    QMessageBox::critical(this, "Error", "Unable to open file");
                    return;
                }

                QImage newImage(filePath);
                QPixmap newPixmap = QPixmap::fromImage(newImage);
                mIcon->setPixmap(newPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        });
    }

    mSidebarLayout->addStretch();

    mIdInput = new QLineEdit(QString::number(rand()));
    mContentLayout->addRow("Id", mIdInput);
    // mContentLayout->addRow("Snapshot", idInput);

    mNameInput = new QLineEdit();
    mNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Name", mNameInput);

    mDescriptionInput = new QLineEdit();
    mDescriptionInput->setPlaceholderText("Describe your item here");
    mContentLayout->addRow("Description", mDescriptionInput);

    mCreatedInput = new QDateTimeEdit();
    mCreatedInput->setDate(QDate::currentDate());
    mContentLayout->addRow("Created", mCreatedInput);

    mUpdatedInput = new QDateTimeEdit();
    mUpdatedInput->setDate(QDate::currentDate());
    mContentLayout->addRow("Updated", mUpdatedInput);

    AddCustomWidgets();

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
    mContentLayout->addWidget(mButtonBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &ItemDialog::OnSave);

    connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
    });
}

EmuDb* ItemDialog::GetDatabase() {
    auto dbProj = dynamic_cast<EmuDbProject*>(mSdk->GetFocusedProject());
    if (dbProj != nullptr)
        return dbProj->GetDb();
    return nullptr;
}