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

#include <curl/curl.h>

#define USE_CUSTOM_STYLESHEET 0

int main(int argc, char *argv[]) {
    int ret = 1;
    QApplication app(argc, argv);
#if defined(__APPLE__) // Native dialogs on Macs look nothing close to how they do on Windows, so we're turning that off.
    app.setAttribute(Qt::ApplicationAttribute::AA_DontUseNativeDialogs, true);
#endif
    CURLcode curlRet = curl_global_init(CURL_GLOBAL_ALL);
    if (curlRet != CURLE_OK) {
        QMessageBox::critical(nullptr, "Error", "Could not initialize curl");
        return curlRet;
    }
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
        goto cleanup;
    launcher.show();
    ret = app.exec();
cleanup:
    curl_global_cleanup();
    return ret;
}