// === noobWarrior ===
// File: BackupDialog.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QSizePolicy>

namespace NoobWarrior {
class BackupDialog : public QDialog {
public:
    enum class ItemSource {
        Undecided,
        OnlineItem,
        LocalFile
    };

    enum class OnlineItemType {
        Undecided,
        Universe,
        Asset,
        User
    };

    BackupDialog(QWidget *parent = nullptr);
    void InitWidgets();
    void UpdateWidgets();

    void InitOnlineUniverseWidgets();
    void InitOnlineAssetWidgets();
    void InitLocalFileWidgets();

    void StartBackup();
private:
    ItemSource mSource;
    OnlineItemType mItemType;

    QVBoxLayout* mMainLayout;
    QVBoxLayout* mFrameLayout;
    QFrame* mFrame;
    QButtonGroup* mItemSourceButtonGroup;
    QHBoxLayout* mItemSourceRowLayout;

    QLabel* mItemTypeCaption;
    QButtonGroup* mItemTypeButtonGroup;
    QHBoxLayout* mItemTypeRowLayout;
    QRadioButton* mItemTypeUniverse;
    QRadioButton* mItemTypeAsset;
    QRadioButton* mItemTypeUser;

    QLabel* mIdCaption;
    QLineEdit* mIdField;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Online Universe Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////
    QFrame* mUniverseFrame;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Online Asset Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Local File Widgets
    ///////////////////////////////////////////////////////////////////////////////////////////////

    QDialogButtonBox* mButtons;
};
}