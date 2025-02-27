// === noobWarrior ===
// File: main.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Main entrypoint for Qt application
#include "Launcher.h"
#include "ArchiveEditor/ArchiveEditor.h"

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QFile>

#define USE_CUSTOM_STYLESHEET 0

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);
    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Bold.ttf");
    NoobWarrior::Launcher launcher;
#if USE_CUSTOM_STYLESHEET
    QFile styleFile(":/css/style.css");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&styleFile);
        app.setStyleSheet(in.readAll());
    }
#endif
    QMessageBox::StandardButton res = QMessageBox::question(nullptr, "Warning",
        "What you are running is incomplete software. Nothing here is suitable for production. Things are bound to change, especially the way critical data is parsed by the program.\n\nBy clicking Yes, you agree to the statement that anything you try to create with this version of the software will eventually be corrupted due to unforeseen consequences.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (res != QMessageBox::Yes)
        return 1;
    launcher.show();
    return app.exec();
}