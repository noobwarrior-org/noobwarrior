// === noobWarrior ===
// File: AboutDialog.h
// Started by: Hattozo
// Started on: 3/15/2025
// Description:
#pragma once

#include <QDialog>

namespace NoobWarrior {
    class AboutDialog : public QDialog {
        Q_OBJECT
    public:
        AboutDialog(QWidget *parent = nullptr);
        ~AboutDialog();
    };
}