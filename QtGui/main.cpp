// === noobWarrior ===
// File: main.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Main entrypoint for Qt application
#include "Launcher.h"
#include "ArchiveEditor/ArchiveEditor.h"

#include <NoobWarrior/Config.h>

#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QFile>

#include <curl/curl.h>

#define USE_CUSTOM_STYLESHEET 0

int main(int argc, char *argv[]) {
    int ret = 1;
    QApplication app(argc, argv);
    CURLcode curlRet = curl_global_init(CURL_GLOBAL_ALL);
    if (curlRet != CURLE_OK) {
        QMessageBox::critical(nullptr, "Error", "Could not initialize curl");
        return curlRet;
    }
    if (NoobWarrior::Config_Open() != 0) {
        QMessageBox::critical(nullptr, "Error", "Could not read config file");
        return 0xC03F16DD; // Kind of reads out as "Config Dead Dead?"
    }
    Q_INIT_RESOURCE(resources);
    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Bold.ttf");
    NoobWarrior::Launcher launcher;
#if USE_CUSTOM_STYLESHEET
    QFile styleFile(":/css/style.css");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&styleFile);
        app.setStyleSheet(in.readAll());
    }
#endif
#if !defined(Q_OS_MACOS)
    QMessageBox::StandardButton res = QMessageBox::question(nullptr, "Warning",
        "What you are running is incomplete software. Nothing here is suitable for production. Things are bound to change, especially the way critical data is parsed by the program.\n\nBy clicking Yes, you agree to the statement that anything you try to create with this version of the software will eventually be corrupted due to unforeseen consequences.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
#else
    QMessageBox msg;
    msg.setText("You are running software that is likely broken");
    msg.setInformativeText("Nothing here is suitable for production. Things are bound to change, especially the way critical data is parsed by the program.\n\nBy clicking Yes, you agree to the statement that anything you try to create with this version of the software will eventually be corrupted due to unforeseen consequences.");
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    int res = msg.exec();
#endif
    if (res != QMessageBox::Yes)
        goto cleanup;
    launcher.show();
    ret = app.exec();
cleanup:
    NoobWarrior::Config_Close();
    curl_global_cleanup();
    return ret;
}