// === noobWarrior ===
// File: ContentEditorDialog.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description: Dialog window that allows you to edit the details of a piece of content.
#pragma once
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/User.h>
#include <NoobWarrior/Database/AssetCategory.h>

#include "ContentEditorDialogBase.h"
#include "DatabaseEditor.h"
#include "IdTypeFields.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include <fstream>
#include <optional>

namespace NoobWarrior {
template<class T>
class ContentEditorDialog : public ContentEditorDialogBase {
public:
    ContentEditorDialog(QWidget *parent = nullptr, const std::optional<int> id = std::nullopt, const std::optional<int> version = std::nullopt) : ContentEditorDialogBase(parent, id, version)
    {
        static_assert(std::is_base_of_v<IdRecord, T>, "typename must inherit from IdRecord");
        ContentEditorDialog<T>::RegenWidgets();
    }

    void RegenWidgets() override {
        ContentEditorDialogBase::RegenWidgets();
        setWindowTitle(mId.has_value() ? tr("Configure %1").arg(T::TableName) : tr("Create New %1").arg(T::TableName));

        ////////////////////////////////////////////////////////////////////////
        /// icon
        ////////////////////////////////////////////////////////////////////////
        QImage image;

        if (mId.has_value()) {
            image.loadFromData(mDatabase->RetrieveContentImageData<T>(mId.value()));

            QPixmap pixmap = QPixmap::fromImage(image);
            mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        if constexpr (!(std::is_same_v<T, Asset> || std::is_same_v<T, User>)) {
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

        ////////////////////////////////////////////////////////////////////////
        /// main content containing all the fields and stuff
        ////////////////////////////////////////////////////////////////////////
        mFields = GetFields<T>();
        if (mId.has_value()) {
            std::optional<T> content = mDatabase->GetContent<T>(mId.value(), mVersion);
            if (content.has_value()) mContent = content.value();
        }
        for (const auto &field : mFields) {
            std::any val = field.ConvertValueToAny(mContent);
            QWidget *widget = field.WidgetFactory(this, val);
            mWidgetFields.push_back(widget);
            mContentLayout->addRow(new QLabel(field.Label, this), widget);
        }
        /*
        auto idInput = new QLineEdit(this);
        idInput->setValidator(new QIntValidator(idInput));
        mContentLayout->addRow(new QLabel(tr("Id"), this), idInput);
        mContentLayout->addRow(new QLabel(tr("Version"), this), new QSpinBox(this));
        mContentLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
        mContentLayout->addRow(new QLabel(tr("Name"), this), new QLineEdit(this));
        if constexpr (std::is_base_of_v<OwnedIdRecord, T>) {
            mContentLayout->addRow(new QLabel(tr("Description"), this), new QLineEdit(this));
            mContentLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
            mContentLayout->addRow(new QLabel(tr("Created"), this), new QDateTimeEdit(this));
            mContentLayout->addRow(new QLabel(tr("Updated"), this), new QDateTimeEdit(this));
        }

        if constexpr (std::is_same_v<T, Asset>) {
            auto *categoryDropdown = new QComboBox(this);
            auto *typeDropdown = new QComboBox(this);

            for (int i = 0; i <= AssetCategoryCount; i++) {
                auto assetTypeCategory = static_cast<AssetCategory>(i);
                QString assetTypeCategoryStr = AssetCategoryAsTranslatableString(assetTypeCategory);
                categoryDropdown->addItem(assetTypeCategoryStr);
            }

            for (int i = 1; i <= Roblox::AssetTypeCount; i++) {
                auto assetType = static_cast<Roblox::AssetType>(i);
                QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
                if (assetTypeStr.compare("None") != 0)
                    typeDropdown->addItem(assetTypeStr, i);
            }

            mContentLayout->addRow(new QLabel(tr("Category"), this), categoryDropdown);
            mContentLayout->addRow(new QLabel(tr("Type"), this), typeDropdown);

            connect(typeDropdown, &QComboBox::currentIndexChanged, [this, typeDropdown](int index) {
                auto assetType = static_cast<Roblox::AssetType>(typeDropdown->currentData().toInt());

                QImage image;
                mId.has_value() ? image.loadFromData(mDatabase->RetrieveContentImageData<T>(mId.value())) :
                    image.loadFromData(GetImageForAssetType(assetType));

                QPixmap pixmap = QPixmap::fromImage(image);
                mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
        }
        */

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

        mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
        mContentLayout->addWidget(mButtonBox);

        connect(mButtonBox, &QDialogButtonBox::accepted, this, [this] () mutable {
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
        });

        connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
            close();
        });
        // mLayout->addRow(new QPushButton("Save", this), new QPushButton("Cancel", this));
    }
private:
    std::any mContent = T {};
    std::vector<FieldDesc> mFields = GetFields<T>();
    std::vector<QWidget*> mWidgetFields;
};
}
