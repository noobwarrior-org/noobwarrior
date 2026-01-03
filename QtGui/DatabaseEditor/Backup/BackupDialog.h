// === noobWarrior ===
// File: BackupDialog.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/Backup.h>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QSizePolicy>

namespace NoobWarrior {
class BackupDialog : public QDialog {
public:
    BackupDialog(QWidget *parent = nullptr);
    void InitWidgets();
    void UpdateWidgets();

    void InitOnlineUniverseWidgets();
    void InitOnlineAssetWidgets();
    void InitLocalFileWidgets();

    void StartBackup();
private:
    bool mChoseItemSource;

    Backup::ItemSource mSource;
    Backup::OnlineItemType mItemType;

    QVBoxLayout* mMainLayout;
    QVBoxLayout* mFrameLayout;
    QFrame* mFrame;
    QButtonGroup* mItemSourceButtonGroup;
    QHBoxLayout* mItemSourceRowLayout;

    QLabel* mItemTypeCaption;
    QComboBox* mItemTypeDropdown;

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