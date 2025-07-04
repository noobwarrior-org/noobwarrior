// === noobWarrior ===
// File: ContentEditorDialog.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <NoobWarrior/Database/Asset.h>
#include <../../Core/Include/NoobWarrior/Database/IdType.h>

#include <QDialog>
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
};
}