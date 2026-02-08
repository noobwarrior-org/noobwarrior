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
#include <QMessageBox>

#include <NoobWarrior/EmuDb/EmuDb.h>

#include <memory>
#include <optional>
#include <fstream>

#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"

namespace NoobWarrior {
template<typename Item>
class ItemDialog : public QDialog {
public:
    ItemDialog(QWidget *parent = nullptr, Item item = {}) : QDialog(parent),
        mItem(item)
    {
        assert(dynamic_cast<Sdk*>(this->parent()) != nullptr && "ItemDialog should not be parented to anything other than Sdk");
        mSdk = dynamic_cast<Sdk*>(this->parent());

        setWindowTitle("Item Editor");
        RegenWidgets();
    }

    virtual void RegenWidgets() {
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

        std::vector<unsigned char> data;

        /*
        if (mId.has_value())
            data = std::move(mDatabase->RetrieveContentImageData(mItemType, mId.has_value() ? mId.value() : -1));
        else
            data.assign(mItemType.DefaultImage, mItemType.DefaultImage + mItemType.DefaultImageSize);
        */

        image.loadFromData(data);

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

        mIdInput = new QLineEdit();
        mContentLayout->addRow("Id", mIdInput);

        /*
        ////////////////////////////////////////////////////////////////////////
        /// main content containing all the fields and stuff
        ////////////////////////////////////////////////////////////////////////
        std::map<std::string, InputWidget> columnNameAndWidgets;
        for (const auto &fieldpair : mItemType.Fields) {
            std::string name = fieldpair.first;
            Reflection::Field field = fieldpair.second;
            Out("ItemDialog", "{} - {}", field.Name, field.Description);

            // These fields are excluded from being automatically included in this list.
            // We instead have our own toggles for editing them, like how Id has mIdInput.
            if (field.Name.compare("Id") == 0 || field.Name.compare("Snapshot") == 0 || field.Name.compare("ImageId") == 0 || field.Name.compare("ImageSnapshot") == 0) {
                continue;
            }

            QWidget* widget = nullptr;

            field.Getter(mItem.get());

            if (field.Type == &typeid(int)) {
                widget = new QLineEdit(this);
                columnNameAndWidgets.emplace(name, static_cast<QLineEdit*>(widget));
            } else if (field.Type == &typeid(std::string)) {
                widget = new QLineEdit(this);
                columnNameAndWidgets.emplace(name, static_cast<QLineEdit*>(widget));
            }

            if (widget != nullptr)
                mContentLayout->addRow(QString::fromStdString(field.PrettyName), widget);

            if (mId.has_value()) {
                // mDatabase->RetrieveColumnsFromItem(mItemType, mId.value(), mSnapshot);
                
                // std::any val = field.Getter(mDatabase, mId.value(), std::nullopt);
                // if (field.Type == &typeid(int)) {
                //     static_cast<QLineEdit*>(widget)->setText(QString::number(std::any_cast<int>(val)));
                // } else if (field.Type == &typeid(std::string)) {
                //     static_cast<QLineEdit*>(widget)->setText(QString::fromStdString(std::any_cast<std::string>(val)));
                // }
            }
        }
        */

        /*
        for (const auto &field : mFields) {
            std::any val = field.ConvertValueToAny(mContent);
            QWidget *widget = field.WidgetFactory(this, val);
            mWidgetFields.push_back(widget);
            mContentLayout->addRow(new QLabel(field.Label, this), widget);
        }

        if constexpr (std::is_same_v<T, Asset>) {
            auto *dataFrame = new QFrame(this);
            mContentLayout->addRow("Data", dataFrame);

            auto *dataLayout = new QVBoxLayout(dataFrame);

            auto *le = new QPushButton();
            le->setText("Select File...");

            auto *label = new QLabel();
            label->setText(QString("Size: %1 bytes").arg(mId.has_value() ? mDatabase->GetAssetSize(mId.value()) : 0));

            dataLayout->addWidget(le);
            dataLayout->addWidget(label);

            connect(le, &QPushButton::clicked, [this]() {
                QString filePath = QFileDialog::getOpenFileName(
                    this,
                    "Choose File",
                    "",
                    "Any File (*.*)"
                );
            });
        }
        */

        mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
        mContentLayout->addWidget(mButtonBox);

        connect(mButtonBox, &QDialogButtonBox::accepted, this, [this] () mutable {
            int64_t newId = static_cast<int64_t>(mIdInput->text().toInt());
            if (newId != mItem.Id) {
                // the user has changed the ID of the item, account for this so that it doesn't try to create an entirely new row.
                // DatabaseResponse res = mDatabase->ChangeItemId(mItemType, mId.value(), newId);
                // if (res != DatabaseResponse::Success) {
                    // this will totally not infuriate people when seen
                    // QMessageBox::critical(this, "Failed to Configure Item", "You changed the ID of the item, but the database failed when trying to internally change the ID.");
                    // return;
                // }
            }
            // mDatabase->GetAssetRepository().Save();

            /*
            // validate everything first
            for (int i = 0; i < mFields.size(); i++) {
                FieldDesc field = mFields[i];
                QWidget *widget = mWidgetFields[i];
                QString errorMsg = field.Validate(this, widget);
                if (!errorMsg.isEmpty()) {
                    if (errorMsg.compare("SILENTFAIL") != 0)
                        QMessageBox::critical(this, "Failed To Add Content", errorMsg);
                    return;
                }
            }

            // and then begin with the value setting
            for (int i = 0; i < mFields.size(); i++) {
                FieldDesc field = mFields[i];
                QWidget *widget = mWidgetFields[i];
                field.SetValue(mContent, widget);
            }

            DatabaseResponse res = mDatabase->AddContent(std::any_cast<T>(mContent), true);
            QString errMsg;
            switch (res) {
                case DatabaseResponse::NotInitialized: errMsg = "Database not initialized"; break;
                case DatabaseResponse::StatementConstraintViolation: errMsg = "A constraint violation occurred when trying to add content to the database."; break;
                default: errMsg = "An unknown error occurred when trying to add content to the database."; break;
            }
            if (res == DatabaseResponse::Success) {
                mDatabaseEditor->Refresh();
                close();
            } else QMessageBox::critical(this, "Failed To Add Content", errMsg);
            */
        });

        connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
            close();
        });
    }

    EmuDb* GetDatabase() {
        auto dbProj = dynamic_cast<EmuDbProject*>(mSdk->GetFocusedProject());
        if (dbProj != nullptr)
            return dbProj->GetDb();
        return nullptr;
    }
protected:
    Item mItem;

    Sdk* mSdk;

    QLabel *mIcon;
    QLineEdit *mIdInput;

    QHBoxLayout *mLayout;
    QVBoxLayout *mSidebarLayout;
    QFormLayout *mContentLayout;

    QDialogButtonBox *mButtonBox;
};
}