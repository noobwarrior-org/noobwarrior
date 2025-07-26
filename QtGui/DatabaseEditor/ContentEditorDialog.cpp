// === noobWarrior ===
// File: ContentEditorDialog.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description: Dialog window that allows you to edit the details of a piece of content.
#include "ContentEditorDialog.h"
#include "DatabaseEditor.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QSpinBox>

using namespace NoobWarrior;

ContentEditorDialog::ContentEditorDialog(IdType idType, QWidget *parent) : QDialog(parent),
    mIdType(idType)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentEditorDialog should not be parented to anything other than DatabaseEditor");
    setWindowTitle(tr("Content Editor"));
    RegenWidgets();
}

void ContentEditorDialog::RegenWidgets() {
    auto editor = dynamic_cast<DatabaseEditor*>(parent());
    Database *db = editor->GetCurrentlyEditingDatabase();

    qDeleteAll(findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    setWindowTitle(tr("Configure %1").arg(IdTypeAsString(mIdType)));

    mLayout = new QFormLayout(this);

    auto idInput = new QLineEdit(this);
    idInput->setValidator(new QIntValidator(idInput));
    mLayout->addRow(new QLabel(tr("Id"), this), idInput);
    mLayout->addRow(new QLabel(tr("Version"), this), new QSpinBox(this));
    mLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
    mLayout->addRow(new QLabel(tr("Name"), this), new QLineEdit(this));
    mLayout->addRow(new QLabel(tr("Description"), this), new QLineEdit(this));
    mLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
    mLayout->addRow(new QLabel(tr("Created"), this), new QDateTimeEdit(this));
    mLayout->addRow(new QLabel(tr("Updated"), this), new QDateTimeEdit(this));

    if (mIdType == IdType::Asset) {
        auto *typeDropdown = new QComboBox(this);

        for (int i = 1; i <= 79; i++) {
            auto assetType = static_cast<Roblox::AssetType>(i);
            QString assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
            if (assetTypeStr.compare("None") != 0)
                typeDropdown->addItem(assetTypeStr);
        }

        mLayout->addRow(new QLabel(tr("Type"), this), typeDropdown);
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
    mLayout->addWidget(mButtonBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, [&, db, editor]() {
        if (mIdType == IdType::Asset) {
            Asset asset {};
            asset.Id = qobject_cast<QLineEdit*>(mLayout->itemAt(0, QFormLayout::FieldRole)->widget())->text().toInt();
            asset.Name = qobject_cast<QLineEdit*>(mLayout->itemAt(3, QFormLayout::FieldRole)->widget())->text().toStdString();
            db->AddAsset(asset);
        }
        editor->GetContentBrowser()->Refresh();
        close();
    });

    connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
    });
    // mLayout->addRow(new QPushButton("Save", this), new QPushButton("Cancel", this));
}
