/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Application.cpp
// Started by: Hattozo
// Started on: 5/18/2025
// Description: Main entrypoint for Qt application
#include "Application.h"
#include "LoadingDialog.h"
#include "Style/DefaultStyle.h"

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/Engine.h>

#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QTimer>
#include <QPointer>
#include <QSocketNotifier>
#include <QMessageBox>
#include <QStyleFactory>

#include <curl/curl.h>
#include <event.h>

#define USE_CUSTOM_STYLE 1

using namespace NoobWarrior;

Application *NoobWarrior::gApp = nullptr;

Application::Application(int &argc, char **argv) : QApplication(argc, argv) {
    mInit.ArgCount = argc;
    mInit.ArgVec = argv;
    mInit.Portable = QDir(QDir(applicationDirPath()).filePath("NW_PORTABLE")).exists();
}

int Application::Run() {
    int ret = 1;

    mCore = new Core(mInit);
    mCore->CreateStandardUserDataDirectories();
    mCore->StartServerEmulator(8080);

    QTimer* evTimer = new QTimer(this);
    evTimer->setTimerType(Qt::CoarseTimer);

    connect(evTimer, &QTimer::timeout, this, [&] {
        int res = mCore->ProcessEvents();
        if (res == 0)
            evTimer->setInterval(0);
        else
            evTimer->setInterval(16); // poll events less if processevents() did not find any work to do
    });
    evTimer->start(0);

    Out("QtApplication", "Finished initializing core, starting Qt application");

    CURLcode curlRet = curl_global_init(CURL_GLOBAL_ALL);
    if (curlRet != CURLE_OK) {
        QMessageBox::critical(nullptr, "Error", "Could not initialize curl");
        return curlRet;
    }

    if (!CheckConfigResponse(mCore->ConfigReturnCode, "Could not read config file.")) return 0xC03F16DD; // Kind of reads out as "Config Dead Dead?"

    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/SourceSansPro-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/FiraMono-Bold.ttf");
#if USE_CUSTOM_STYLE
    /*
    QFile styleFile(":/css/style.css");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&styleFile);
        setStyleSheet(in.readAll());
    }
    */
    QApplication::setStyle(new DefaultStyle());
#else
    #if defined(Q_OS_WIN32)
        QApplication::setStyle(QStyleFactory::create("windowsvista")); // set it to the vista one because the windows 11 theme is fucking disgusting
    #endif
#endif
    mLauncher = new Launcher();
    QMessageBox msg;
#if !defined(Q_OS_MACOS)
    msg.setText("Warning");
    msg.setInformativeText("What you are running is incomplete software. Nothing here is suitable for production. Things are bound to change, especially the way critical data is parsed by the program.\n\nBy clicking Yes, you agree to the statement that anything you try to create with this version of the software will eventually be corrupted due to unforeseen consequences.");
    msg.setIcon(QMessageBox::Information);
#else
    msg.setText("You are running software that is likely broken");
    msg.setInformativeText("Nothing here is suitable for production. Things are bound to change, especially the way critical data is parsed by the program.\n\nBy clicking Yes, you agree to the statement that anything you try to create with this version of the software will eventually be corrupted due to unforeseen consequences.");
#endif
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    int res = msg.exec();
    if (res != QMessageBox::Yes)
        goto cleanup;
    mLauncher->show();
    ret = exec();
cleanup:
    Out("QtApplication", "Cleaning up!");

    mLauncher->deleteLater();
    mLauncher = nullptr;

    curl_global_cleanup();
    mCore->StopServerEmulator();

    NOOBWARRIOR_FREE_PTR(mCore)
    return ret;
}

Core *Application::GetCore() {
    return mCore;
}

bool Application::CheckConfigResponse(ConfigResponse res, const QString &errStr) {
    if (res != ConfigResponse::Success) {
        std::string reasonMsg;
        switch (res) {
            default: reasonMsg = "The reason for this is unknown."; break;
            case ConfigResponse::CantReadFile:
                reasonMsg = "The contents cannot be read from it. Check if you have the appropriate permissions to be able to read from it.";
                break;
            case ConfigResponse::SyntaxError:
                reasonMsg = "It has syntax errors and could not be parsed correctly. Delete the file so that it is regenerated by the program, or fix any syntax errors that are preventing it from being parsed correctly.";
                break;
            case ConfigResponse::MemoryError:
                reasonMsg = "An error occurred when trying to allocate memory to read the file.";
                break;
            case ConfigResponse::ErrorDuringExecution:
                reasonMsg = "Errors were encountered when executing the config file.";
                break;
            case ConfigResponse::ReturningWrongType:
                reasonMsg = "It is not returning a table.";
                break;
        }
        QMessageBox::critical(nullptr, "Error",
            QString("%1 %2\n\nError given by Lua: \"%3\"")
                .arg(errStr)
                .arg(QString::fromStdString(reasonMsg))
                .arg(QString::fromStdString(GetCore()->GetConfig()->GetLuaError()))
        );
        return false; // Kind of reads out as "Config Dead Dead?"
    }
    return true;
}

void Application::DownloadAndInstallWine(std::function<void(bool)> callback) {
    Out("Application", "Installing wine");
    auto *dialog = new LoadingDialog(nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
}

void Application::DownloadAndInstallEngine(const Engine &client, std::function<void(bool)> callback) {
    Out("Application", "Installing client {}", client.Version);
    auto *dialog = new LoadingDialog(nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);

    auto transfers = std::make_shared<std::vector<std::shared_ptr<Transfer>>>();;

    QObject::connect(dialog, &QWidget::destroyed, [transfers]() {
        for (auto &t : *transfers) {
            if (t && t->Cancelled) t->Cancelled->store(true);
        }
    });

    dialog->show();

    QPointer<LoadingDialog> dialogPtr(dialog);

    auto install_callback = std::make_shared<std::function<void(EngineInstallState, CURLcode, size_t, size_t)>>(
        [=](EngineInstallState state, CURLcode code, size_t size, size_t totalSize) -> void {
            double sizeMb = static_cast<double>(size) / (1024 * 1024);
            double totalSizeMb = static_cast<double>(totalSize) / (1024 * 1024);

            // This callback is actually running on another thread, so lets use this QTimer thing to make it run on the main thread.
            QTimer::singleShot(0, dialogPtr.data(), [=]() {
                if (!dialogPtr) return;

                switch (state) {
                default: break;
                case EngineInstallState::RetrievingIndex:
                    dialogPtr->SetText("Retrieving index...");
                    dialogPtr->SetProgress(-1);
                    break;
                case EngineInstallState::DownloadingFiles:
                    dialogPtr->SetText(QString("Downloading Roblox %1 %2 (%3 MB/%4 MB)").arg(QString::fromUtf8(EngineSideAsTranslatableString(client.Side)), QString::fromStdString(client.Version), QString::number(sizeMb, 'f', 1), QString::number(totalSizeMb, 'f', 1)));
                    if (totalSizeMb > 0) // pls dont ever divide by 0
                        dialogPtr->SetProgress(sizeMb / totalSizeMb);
                    break;
                case EngineInstallState::ExtractingFiles:
                    dialogPtr->SetText("Extracting files...");
                    dialogPtr->SetProgress(-1);
                    break;
                }

                if (state == EngineInstallState::Failed || state == EngineInstallState::Success) {
                    if (state == EngineInstallState::Failed) QMessageBox::critical(nullptr, "Failed To Download Client", "An error has occurred!");
                    dialogPtr->close();
                    callback(state == EngineInstallState::Success);
                }
            });
        }
    );

    mCore->DownloadAndInstallEngine(client, transfers, install_callback);
}

void Application::LaunchEngine(const Engine &engine) {
    std::function callback = [this, engine](bool success) {
        if (!success) return;
        
        auto *dialog = new LoadingDialog(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setModal(false);
        dialog->SetText(QString("Loading Roblox %1 %2...").arg(QString::fromUtf8(EngineSideAsTranslatableString(engine.Side)), QString::fromStdString(engine.Version)));
        dialog->DisableCancel(true);
        dialog->show();

        EngineLaunchResponse res = mCore->LaunchEngine(engine);
        if (res != EngineLaunchResponse::Success) {
            QString errMsg;
            switch (res) {
            default: errMsg = "An error occurred while trying to launch Roblox."; break;
            case EngineLaunchResponse::NotInstalled: errMsg = "The engine that you are trying to launch is not installed on your computer. Please install it and try again."; break;
            case EngineLaunchResponse::NoValidExecutable: errMsg = "Could not find a valid executable for the version of Roblox that you are trying to launch. Please re-install and try again."; break;
            case EngineLaunchResponse::FailedToCreateProcess: errMsg = "Could not create a valid process for Roblox."; break;
            case EngineLaunchResponse::InjectFailed: errMsg = "Failed to inject into the Roblox process."; break;
            case EngineLaunchResponse::InjectDllMissing: errMsg = "Failed to locate DLL file. Please make sure it's in the right place."; break;
            case EngineLaunchResponse::InjectCannotAccessProcess: errMsg = "Could not access the Roblox process in order to perform DLL injection. Do you have a kernel-level anti-cheat running?"; break;
            case EngineLaunchResponse::InjectWrongArchitecture: errMsg = "Tried injecting 64-bit DLL into 32-bit process. If you are on a 32-bit version of Windows, this error message is misleading. Feel free to fix it!"; break;
            case EngineLaunchResponse::InjectCannotWriteToProcessMemory: errMsg = "Could not write arbitrary memory to the Roblox process."; break;
            case EngineLaunchResponse::InjectCannotCreateThreadInProcess: errMsg = "Could not create a thread in the Roblox process."; break;
            case EngineLaunchResponse::InjectCouldNotGetReturnValueOfLoadLibrary: errMsg = "Could not get the return value of the LoadLibrary API call."; break;
            case EngineLaunchResponse::InjectFailedToLoadLibrary: errMsg = "Failed to load the DLL file. Please make sure that it's in the right place and see if the version of Roblox you're using is supported."; break;
            case EngineLaunchResponse::InjectFailedToResumeProcess: errMsg = "Failed to resume Roblox process after injecting DLL."; break;
            }
            QMessageBox::critical(dialog, "Cannot Launch Engine", errMsg);
            dialog->close();
        }
    };

    if (!mCore->IsEngineInstalled(engine)) {
        DownloadAndInstallEngine(engine, callback);
    } else callback(true);
}

int main(int argc, char **argv) {
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(shared_resources); // you must do this or else the compiler will optimize it out of the code.
    Application app(argc, argv);
    gApp = &app;
    return app.Run();
}
