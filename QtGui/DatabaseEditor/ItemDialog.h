// === noobWarrior ===
// File: ItemDialog.h
// Started by: Hattozo
// Started on: 10/27/2025
// Description: Dialog window that allows you to edit or create an item.
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Reflection.h>

#include "DatabaseEditor.h"

namespace NoobWarrior {
class ItemDialog : public QDialog {
    Q_OBJECT
public:
    ItemDialog(QWidget *parent = nullptr, Reflection::IdType &idType = Reflection::GetIdType<Asset>(), const std::optional<int> id = std::nullopt, const std::optional<int> version = std::nullopt);
    virtual void RegenWidgets();
    std::optional<int> GetId();
    std::optional<int> GetVersion();
    Database *GetDatabase();

    QLabel *mIcon;
protected:
    Reflection::IdType mIdType;
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