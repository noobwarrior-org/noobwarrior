// === noobWarrior ===
// File: ContentEditorDialog.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description: Dialog window that allows you to edit the details of a piece of content.
#pragma once
#include <NoobWarrior/Database/Record/IdType/Asset.h>
#include <NoobWarrior/Database/Record/IdType/User.h>
#include <NoobWarrior/Database/AssetCategory.h>

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
class ContentEditorDialog : public QDialog {
public:
    ContentEditorDialog(QWidget *parent = nullptr, std::optional<int> id = std::nullopt) : QDialog(parent),
        mId(id)
    {
        assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentEditorDialog should not be parented to anything other than DatabaseEditor");
        mDatabaseEditor = dynamic_cast<DatabaseEditor*>(this->parent());
        mDatabase = mDatabaseEditor->GetCurrentlyEditingDatabase();

        setWindowTitle(tr("Content Editor"));
        RegenWidgets();
    }

    void RegenWidgets() {
        qDeleteAll(findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
        setWindowTitle(mId.has_value() ? tr("Configure %1").arg(T::TableName) : tr("Create New %1").arg(T::TableName));



        mLayout = new QHBoxLayout(this);
        mSidebarLayout = new QVBoxLayout();
        mContentLayout = new QFormLayout();

        mLayout->addLayout(mSidebarLayout);
        mLayout->addLayout(mContentLayout);

        ////////////////////////////////////////////////////////////////////////
        /// icon
        ////////////////////////////////////////////////////////////////////////
        QImage image;

        if (mId.has_value())
            image.loadFromData(mDatabase->RetrieveContentImageData<T>(mId.value()));

        QPixmap pixmap = QPixmap::fromImage(image);

        mIcon = new QLabel();
        mIcon->setAlignment(Qt::AlignLeft);
        mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        mSidebarLayout->addWidget(mIcon);

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
        std::vector<FieldDesc> fields = GetIdTypeFields<T>();
        for (const auto &field : fields) {
            QWidget *poop = field.WidgetFactory(this);
            mContentLayout->addRow(new QLabel(field.Label, this), poop);
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

        mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
        mContentLayout->addWidget(mButtonBox);

        connect(mButtonBox, &QDialogButtonBox::accepted, this, [this]() {
            T content {};
            content.Id = qobject_cast<QLineEdit*>(mContentLayout->itemAt(0, QFormLayout::FieldRole)->widget())->text().toInt();
            content.Name = qobject_cast<QLineEdit*>(mContentLayout->itemAt(3, QFormLayout::FieldRole)->widget())->text().toStdString();
            if constexpr (std::is_same_v<T, Asset>) {
                // content.Type = qobject_cast<QComboBox*>()
            }
            DatabaseResponse res = mDatabase->AddContent(content);
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
    std::optional<int> mId;
    DatabaseEditor *mDatabaseEditor;
    Database *mDatabase;

    QHBoxLayout *mLayout;
    QVBoxLayout *mSidebarLayout;
    QFormLayout *mContentLayout;

    QLabel *mIcon;
    QDialogButtonBox *mButtonBox;
};
}
