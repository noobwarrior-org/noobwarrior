// === noobWarrior ===
// File: ContentEditorDialog.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description: Dialog window that allows you to edit the details of a piece of content.
#include "ContentEditorDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QSpinBox>

using namespace NoobWarrior;

ContentEditorDialog::ContentEditorDialog(IdType idType, QWidget *parent) : QDialog(parent),
    mIdType(idType)
{
    setWindowTitle(tr("Content Editor"));
    RegenWidgets();
}

void ContentEditorDialog::RegenWidgets() {
    qDeleteAll(findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    setWindowTitle(tr("Configure %1").arg(IdTypeAsString(mIdType)));

    mLayout = new QFormLayout(this);

    mLayout->addRow(new QLabel(tr("Id"), this), new QLineEdit(this));
    mLayout->addRow(new QLabel(tr("Version"), this), new QSpinBox(this));
    mLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
    mLayout->addRow(new QLabel(tr("Name"), this), new QLineEdit(this));
    mLayout->addRow(new QLabel(tr("Description"), this), new QLineEdit(this));
    mLayout->addItem(new QSpacerItem(16, 16, QSizePolicy::Minimum, QSizePolicy::Minimum));
    mLayout->addRow(new QLabel(tr("Created"), this), new QDateTimeEdit(this));
    mLayout->addRow(new QLabel(tr("Updated"), this), new QDateTimeEdit(this));

    if (mIdType == IdType::Asset) {
        mLayout->addRow(new QLabel(tr("Id"), this), new QLineEdit(this));
    }

    mLayout->addRow(new QPushButton("Save", this), new QPushButton("Cancel", this));
}
