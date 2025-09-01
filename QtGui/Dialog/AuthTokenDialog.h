// === noobWarrior ===
// File: AuthTokenDialog.h
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include "AuthBaseDialog.h"

#include <QLineEdit>

namespace NoobWarrior {
class AuthTokenDialog : public AuthBaseDialog {
public:
    static QString GetSourceURL();

    AuthTokenDialog(QWidget *parent = nullptr);
    void InitWidgets() override;
private:
    QLabel *TitleLabel;
    QLineEdit *TokenInput;
};
}