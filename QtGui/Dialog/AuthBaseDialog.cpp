// === noobWarrior ===
// File: AuthBaseDialog.cpp
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include "AuthBaseDialog.h"

using namespace NoobWarrior;

QString AuthBaseDialog::GetSourceURL() {
    return "https://github.com/noobwarrior-org/noobwarrior/blob/master/QtGui/Dialog/AuthBaseDialog.cpp";
}

AuthBaseDialog::AuthBaseDialog(QWidget *parent) : QDialog(parent), Layout(this) {
    setWindowTitle("Authenticate Roblox Account");
}

void AuthBaseDialog::InitWidgets() {
    ButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    Layout.addWidget(ButtonBox);

    auto *lol = new QLabel(QString("Don't trust us? Don't log in, or use a disposable account instead.<br>Or if you're a programmer, read the source code of this dialog form here: <a href=\"%1\">%1</a><br>You should also make sure that your copy of noobWarrior has not been tampered with.").arg(GetSourceURL()), this);
    lol->setTextFormat(Qt::RichText);
    lol->setTextInteractionFlags(Qt::TextBrowserInteraction);
    lol->setOpenExternalLinks(true);
    lol->setWordWrap(true);

    QFont font = lol->font();
    font.setPointSize(9);
    lol->setFont(font);
    lol->setStyleSheet("QLabel { color: gray; }");

    Layout.addWidget(lol);
}