// === noobWarrior ===
// File: Launcher.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: This is the startup window that appears when the user launches the application.
#include <NoobWarrior/NoobWarrior.h>

#include "Launcher.h"
#include "Ui/ui_Launcher.h"

#include <QLabel>

using namespace NoobWarrior;

Launcher::Launcher(QWidget *parent) : QDialog(parent), ui(new Ui::Launcher), mArchiveEditor(nullptr) {
    ui->setupUi(this);
    connect(ui->archiveEditor, &QPushButton::clicked, [&]() {
        if (!mArchiveEditor) {
            mArchiveEditor = new ArchiveEditor(nullptr);
            mArchiveEditor->setAttribute(Qt::WA_DeleteOnClose); // Ensure deletion on close

            QObject::connect(mArchiveEditor, &QWidget::destroyed, [&]() {
                mArchiveEditor = nullptr;
            });

            mArchiveEditor->show();
        } else {
            mArchiveEditor->raise(); // Bring to front if already open
            mArchiveEditor->activateWindow(); // Make active
        }
    });
}

Launcher::~Launcher() {
    delete ui;
    ui = nullptr;
}