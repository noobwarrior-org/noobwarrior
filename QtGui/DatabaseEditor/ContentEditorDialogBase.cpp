// === noobWarrior ===
// File: ContentEditorDialogBase.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description: Base class for ContentEditorDialog. This is not a template class and is generic.
#include "ContentEditorDialogBase.h"

using namespace NoobWarrior;

ContentEditorDialogBase::ContentEditorDialogBase(QWidget *parent, const std::optional<int> id, const std::optional<int> version) : QDialog(parent),
    mId(id),
    mVersion(version)
{
    assert(dynamic_cast<DatabaseEditor*>(this->parent()) != nullptr && "ContentEditorDialog should not be parented to anything other than DatabaseEditor");
    mDatabaseEditor = dynamic_cast<DatabaseEditor*>(this->parent());
    mDatabase = mDatabaseEditor->GetCurrentlyEditingDatabase();

    setWindowTitle(tr("Content Editor"));
}

void ContentEditorDialogBase::RegenWidgets() {
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
}

std::optional<int> ContentEditorDialogBase::GetId() {
    return mId;
}

std::optional<int> ContentEditorDialogBase::GetVersion() {
    return mVersion;
}

Database *ContentEditorDialogBase::GetDatabase() {
    return mDatabase;
}
