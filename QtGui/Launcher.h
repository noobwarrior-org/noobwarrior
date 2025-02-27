// === noobWarrior ===
// File: Launcher.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description:
#pragma once

#include "ArchiveEditor/ArchiveEditor.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class Launcher; }
QT_END_NAMESPACE

namespace NoobWarrior {
    class Launcher : public QDialog {
        Q_OBJECT

    public:
        Launcher(QWidget *parent = nullptr);
        ~Launcher();

    private:
        Ui::Launcher *ui;

        ArchiveEditor *mArchiveEditor;
    };
}