// === noobWarrior ===
// File: AuthBaseDialog.h
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

namespace NoobWarrior {
class AuthBaseDialog : public QDialog {
public:
    static QString GetSourceURL();

    AuthBaseDialog(QWidget *parent = nullptr);
    virtual void InitWidgets();
protected:
    QVBoxLayout Layout;
    QDialogButtonBox *ButtonBox;
};
}