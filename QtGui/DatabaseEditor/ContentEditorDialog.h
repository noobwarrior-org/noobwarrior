// === noobWarrior ===
// File: ContentEditorDialog.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <../../Core/Include/NoobWarrior/Database/IdType/Asset.h>
#include <../../Core/Include/NoobWarrior/Database/IdType/IdType.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>

namespace NoobWarrior {
class ContentEditorDialog : public QDialog {
    Q_OBJECT
public:
    ContentEditorDialog(IdType idType = IdType::Asset, QWidget *parent = nullptr);

    void RegenWidgets();
private:
    IdType mIdType;
    QFormLayout *mLayout;
    QDialogButtonBox *mButtonBox;
};
}