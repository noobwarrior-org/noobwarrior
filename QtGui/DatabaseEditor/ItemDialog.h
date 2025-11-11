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
    ItemDialog(QWidget *parent = nullptr, const Reflection::IdType &idType = Reflection::GetIdType<Asset>(), const std::optional<int64_t> id = std::nullopt, const std::optional<int> snapshot = std::nullopt);
    virtual void RegenWidgets();
    std::optional<int> GetId();
    std::optional<int> GetSnapshot();
    Database *GetDatabase();
protected:
    const Reflection::IdType &mIdType;
    std::optional<int> mId;
    std::optional<int> mSnapshot;

    DatabaseEditor *mDatabaseEditor;
    Database *mDatabase;

    QLabel *mIcon;
    QLineEdit *mIdInput;

    QHBoxLayout *mLayout;
    QVBoxLayout *mSidebarLayout;
    QFormLayout *mContentLayout;

    QDialogButtonBox *mButtonBox;
};
}