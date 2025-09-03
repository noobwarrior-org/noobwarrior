// === noobWarrior ===
// File: ContentBackupDialog.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <QDialog>

namespace NoobWarrior {
template<class T>
class ContentBackupDialog : public QDialog {
public:
    ContentBackupDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(tr("Backup %1").arg(T::TableName));
    }
};
}