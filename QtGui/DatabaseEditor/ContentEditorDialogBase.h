// === noobWarrior ===
// File: ContentEditorDialogBase.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description: Base class for ContentEditorDialog. This is not a template class and is generic.
#pragma once
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "DatabaseEditor.h"

#include <NoobWarrior/Database/Database.h>

namespace NoobWarrior {
class ContentEditorDialogBase : public QDialog {
public:
    ContentEditorDialogBase(QWidget *parent = nullptr, const std::optional<int> id = std::nullopt, const std::optional<int> version = std::nullopt);
    virtual void RegenWidgets();
    std::optional<int> GetId();
    std::optional<int> GetVersion();
    Database *GetDatabase();

    QLabel *mIcon;
protected:
    std::optional<int> mId;
    std::optional<int> mVersion;

    DatabaseEditor *mDatabaseEditor;
    Database *mDatabase;

    QHBoxLayout *mLayout;
    QVBoxLayout *mSidebarLayout;
    QFormLayout *mContentLayout;

    QDialogButtonBox *mButtonBox;
};
}
