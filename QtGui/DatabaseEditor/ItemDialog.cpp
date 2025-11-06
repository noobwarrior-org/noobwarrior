// === noobWarrior ===
// File: ItemDialog.cpp
// Started by: Hattozo
// Started on: 10/27/2025
// Description: Dialog window that allows you to edit or create an item.
#include "ItemDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <fstream>

using namespace NoobWarrior;

ItemDialog::ItemDialog(QWidget *parent, const Reflection::IdType &idType, const std::optional<int64_t> id, const std::optional<int> version) : QDialog(parent),
    mIdType(idType),
    mId(id),
    mVersion(version)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentEditorDialog should not be parented to anything other than DatabaseEditor");
    mDatabaseEditor = dynamic_cast<DatabaseEditor*>(this->parent());
    mDatabase = mDatabaseEditor->GetCurrentlyEditingDatabase();

    setWindowTitle("Item Editor");
    RegenWidgets();
}

void ItemDialog::RegenWidgets() {
    setWindowTitle(mId.has_value() ? tr("Configure %1").arg(QString::fromStdString(mIdType.Name)) : tr("Create New %1").arg(QString::fromStdString(mIdType.Name)));

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

    if (mId.has_value()) {
        data = std::move(mDatabase->RetrieveContentImageData(mIdType, mId.has_value() ? mId.value() : -1));
    } else {
        data.assign(mIdType.DefaultImage, mIdType.DefaultImage + mIdType.DefaultImageSize);
    }

    image.loadFromData(data);

    QPixmap pixmap = QPixmap::fromImage(image);
    mIcon->setPixmap(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (!(mIdType.Name.compare("Asset") == 0 || mIdType.Name.compare("User") == 0)) {
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

    Out("ItemDialog", "id type name {}", mIdType.Name);

    ////////////////////////////////////////////////////////////////////////
    /// main content containing all the fields and stuff
    ////////////////////////////////////////////////////////////////////////
    for (const auto &fieldpair : mIdType.Fields) {
        std::string name = fieldpair.first;
        Reflection::Field field = fieldpair.second;
        Out("ItemDialog", "{} - {}", field.Name, field.Description);

        if (field.Name.compare("ImageId") == 0 || field.Name.compare("ImageVersion") == 0) {
            continue;
        }

        void* widget = nullptr;

        if (field.Type == &typeid(int)) {
            widget = new QLineEdit(this);
        } else if (field.Type == &typeid(std::string)) {
            widget = new QLineEdit(this);
        }

        if (widget != nullptr)
            mContentLayout->addRow(QString::fromStdString(field.PrettyName), static_cast<QWidget*>(widget));

        if (mId.has_value()) {
            std::any val = field.Getter(mDatabase, mId.value(), std::nullopt);
            if (field.Type == &typeid(int)) {
                static_cast<QLineEdit*>(widget)->setText(QString::number(std::any_cast<int>(val)));
            } else if (field.Type == &typeid(std::string)) {
                static_cast<QLineEdit*>(widget)->setText(QString::fromStdString(std::any_cast<std::string>(val)));
            }
        }
    }

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

std::optional<int> ItemDialog::GetId() {
    return mId;
}

std::optional<int> ItemDialog::GetVersion() {
    return mVersion;
}

Database *ItemDialog::GetDatabase() {
    return mDatabase;
}
