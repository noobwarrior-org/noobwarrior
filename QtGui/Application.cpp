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
#include <NoobWarrior/Registry.h>
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
#include <qnamespace.h>
#include <qsystemtrayicon.h>

#define USE_CUSTOM_STYLE 1

using namespace NoobWarrior;

Application *NoobWarrior::gApp = nullptr;

Application::Application(int &argc, char **argv) : QApplication(argc, argv),
    mLauncher(nullptr)
{
    mInit.ArgCount = argc;
    mInit.ArgVec = argv;
    mInit.Portable = QDir(applicationDirPath()).exists("NW_PORTABLE");
    mInit.InstallDataRelativePath = "";
}

int Application::Run() {
    int ret = 1;

    mCore = new Core(mInit);

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

    if (!CheckConfigResponse(mCore->RegistryReturnCode, "Could not read config file.")) return 0xC03F16DD; // Kind of reads out as "Config Dead Dead?"

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

    mTrayMenu = new QMenu();
        QAction* rbxAcc = mTrayMenu->addAction("");
        rbxAcc->setDisabled(true);
    mTrayMenu->addSeparator();
        QAction* robloxProcessesAction = mTrayMenu->addAction("0 Running Roblox Processes");
        robloxProcessesAction->setDisabled(true);
    mTrayMenu->addSeparator();
        QAction* openLauncherAction = mTrayMenu->addAction(QIcon(":/images/silk/application_view_list.png"), "Open Launcher", [this]() {
            if (mLauncher == nullptr) {
                mLauncher = new Launcher();
                mLauncher->setAttribute(Qt::WA_DeleteOnClose);
                connect(mLauncher, &QDialog::destroyed, [this]() {
                    mLauncher = nullptr;
                });
                mLauncher->show();
            } else {
                mLauncher->activateWindow();
            }
        });
    mTrayMenu->addSeparator();
        mTrayMenu->addAction(QIcon(":/images/silk/cross.png"), "Quit", [this]() {
            this->exit();
        });
    mTrayIcon = new QSystemTrayIcon(this);
    mTrayIcon->setToolTip("noobWarrior");
    mTrayIcon->setContextMenu(mTrayMenu);

    connect(mTrayIcon, &QSystemTrayIcon::activated, [this, rbxAcc, openLauncherAction](QSystemTrayIcon::ActivationReason reason) {
#if !defined(Q_OS_MACOS)
        if (reason == QSystemTrayIcon::Trigger) {
            openLauncherAction->trigger();
        } else if (reason == QSystemTrayIcon::Context) {
#endif
            rbxAcc->setText(mCore->GetRbxKeychain()->IsLoggedIn() ?
                QString("Roblox Account: ") + QString::fromStdString(mCore->GetRbxKeychain()->GetActiveAccount()->Name) :
                "Not logged into Roblox"
            );
#if !defined(Q_OS_MACOS)
        }
#endif
    });

#if !defined(Q_OS_MACOS)
    auto appIcon = QIcon(":/images/icon16_aa.png");
#else
    auto appIcon = QIcon(":/images/tray_mac.png");
    appIcon.setIsMask(true);
#endif
    mTrayIcon->setIcon(appIcon);

    mTrayIcon->show();

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
    openLauncherAction->trigger();
    ret = exec();
cleanup:
    Out("QtApplication", "Cleaning up!");

    if (mLauncher != nullptr)
        mLauncher->deleteLater();
    mTrayMenu->deleteLater();

    curl_global_cleanup();
    mCore->StopServerEmulator();

    NOOBWARRIOR_FREE_PTR(mCore)
    return ret;
}

Core *Application::GetCore() {
    return mCore;
}

bool Application::CheckConfigResponse(RegistryResponse res, const QString &errStr) {
    if (res != RegistryResponse::Success) {
        std::string reasonMsg;
        switch (res) {
            default: reasonMsg = "The reason for this is unknown."; break;
            case RegistryResponse::CantReadFile:
                reasonMsg = "The contents cannot be read from it. Check if you have the appropriate permissions to be able to read from it.";
                break;
            case RegistryResponse::SyntaxError:
                reasonMsg = "It has syntax errors and could not be parsed correctly. Delete the file so that it is regenerated by the program, or fix any syntax errors that are preventing it from being parsed correctly.";
                break;
            case RegistryResponse::MemoryError:
                reasonMsg = "An error occurred when trying to allocate memory to read the file.";
                break;
            case RegistryResponse::ErrorDuringExecution:
                reasonMsg = "Errors were encountered when executing the config file.";
                break;
            case RegistryResponse::ReturningWrongType:
                reasonMsg = "It is not returning a table.";
                break;
        }
        QMessageBox::critical(nullptr, "Error",
            QString("%1 %2\n\nError given by Lua: \"%3\"")
                .arg(errStr)
                .arg(QString::fromStdString(reasonMsg))
                .arg(QString::fromStdString(GetCore()->GetRegistry()->GetLuaError()))
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

    connect(dialog, &QWidget::destroyed, [transfers]() {
        for (auto &t : *transfers) {
            if (t && t->Canceled) t->Canceled->store(true);
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
                    dialogPtr->SetText(QString("Downloading Roblox %1 %2 (%3 MB/%4 MB)").arg(QString::fromUtf8(EngineSideAsString(client.Side)), QString::fromStdString(client.Version), QString::number(sizeMb, 'f', 1), QString::number(totalSizeMb, 'f', 1)));
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

void Application::LaunchEngine(EngineStartParameters params) {
    std::function callback = [this, params](bool success) {
        if (!success) return;

        auto *dialog = new LoadingDialog(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setModal(false);
        dialog->SetText(QString("Loading Roblox %1 %2...").arg(QString::fromUtf8(EngineSideAsString(params.Engine.Side)), QString::fromStdString(params.Engine.Version)));
        dialog->DisableCancel(true);
        dialog->show();

        EngineLaunchResponse res = mCore->LaunchEngine(params);
        if (res != EngineLaunchResponse::Success) {
            QString errMsg;
            switch (res) {
            default: errMsg = QString("An error occurred while trying to launch Roblox. (code %1)").arg(static_cast<int>(res)); break;
            case EngineLaunchResponse::Failed: errMsg = "The injector process finished but the launcher could not retrieve its exit code."; break;
            case EngineLaunchResponse::NotInstalled: errMsg = "The engine that you are trying to launch is not installed on your computer. Please install it and try again."; break;
            case EngineLaunchResponse::NoValidExecutable: errMsg = "Could not find a valid executable for the version of Roblox that you are trying to launch. Please re-install and try again."; break;
            case EngineLaunchResponse::FailedToCreateProcess: errMsg = "Could not create an injector process. Check if noobhook_x86_injector.exe is inside of the noobWarrior installation folder."; break;
            case EngineLaunchResponse::InjectFailed: errMsg = "Failed to inject into the Roblox process."; break;
            case EngineLaunchResponse::InjectDllMissing: errMsg = "Failed to locate DLL file. Please make sure it's in the right place."; break;
            case EngineLaunchResponse::InjectCannotAccessProcess: errMsg = "Could not access the Roblox process in order to perform DLL injection. Do you have a kernel-level anti-cheat running?"; break;
            case EngineLaunchResponse::InjectWrongArchitecture: errMsg = "Tried injecting 64-bit DLL into 32-bit process. If you are on a 32-bit version of Windows, this error message is misleading. Feel free to fix it!"; break;
            case EngineLaunchResponse::InjectCannotWriteToProcessMemory: errMsg = "Could not write arbitrary memory to the Roblox process."; break;
            case EngineLaunchResponse::InjectFailedToGetModuleHandle: errMsg = "The injector could not resolve a required module handle inside the target process."; break;
            case EngineLaunchResponse::InjectFailedToGetFunctionAddress: errMsg = "The injector could not locate a required function inside the target process."; break;
            case EngineLaunchResponse::InjectCannotCreateThreadInProcess: errMsg = "Could not create a thread in the Roblox process."; break;
            case EngineLaunchResponse::InjectThreadTimedOut: errMsg = "The injected thread in the Roblox process timed out before completing."; break;
            case EngineLaunchResponse::InjectCouldNotGetReturnValueOfLoadLibrary: errMsg = "Could not get the return value of the LoadLibrary API call."; break;
            case EngineLaunchResponse::InjectFailedToLoadLibrary: errMsg = "Failed to load the DLL file. Please make sure that it's in the right place and see if the version of Roblox you're using is supported."; break;
            case EngineLaunchResponse::InjectFailedToResumeProcess: errMsg = "Failed to resume Roblox process after injecting DLL."; break;
            }
            QMessageBox::critical(dialog, "Cannot Launch Engine", errMsg);
            dialog->close();
        } else {
            QPointer<LoadingDialog> dialogPtr(dialog);

            QTimer::singleShot(5000, [dialogPtr]() {
                if (dialogPtr) {
                    dialogPtr->deleteLater();
                }
            });
        }
    };

    if (!mCore->IsEngineInManifest(params.Engine)) {
        Out("LaunchEngine", "Engine not in manifest!");
        DownloadAndInstallEngine(params.Engine, callback);
    } else callback(true);
}

void Application::ConnectToServer(const std::string &ip, uint16_t port) {
    auto *dialog = new LoadingDialog(nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->SetText(QString("Connecting to server %1:%2...").arg(QString::fromStdString(ip), QString::number(port)));
    dialog->show();

    auto cancelled = std::make_shared<bool>(false);
    QPointer<LoadingDialog> dialogPtr(dialog);

    connect(dialog, &QWidget::destroyed, [cancelled]() {
        *cancelled = true;
    });

    mCore->ConnectToServerEmulator(ip, port, [this, dialogPtr, cancelled](ServerEmulatorConnectFailReason failReason, std::vector<EngineStartParameters> availableServers) mutable {
        QTimer::singleShot(0, qApp, [this, dialogPtr, cancelled, failReason, availableServers]() mutable {
            if (*cancelled || !dialogPtr) return;

            if (failReason != ServerEmulatorConnectFailReason::None) {
                QString reasonMsg;
                switch (failReason) {
                default: reasonMsg = "The reason is unknown."; break;
                case ServerEmulatorConnectFailReason::TimedOut:
                    reasonMsg = "The connection timed out."; break;
                case ServerEmulatorConnectFailReason::EndpointNotFound:
                    reasonMsg = "The endpoint /v1/running-game-servers could not be found."; break;
                case ServerEmulatorConnectFailReason::JsonFailed:
                    reasonMsg = "Failed to parse JSON."; break;
                }
                QMessageBox::critical(dialogPtr, "Cannot Connect",
                    QString("Failed to connect to server emulator.\n%1").arg(reasonMsg));
                dialogPtr->close();
                return;
            }
            if (availableServers.empty()) {
                QMessageBox::critical(dialogPtr, "No Running Game Servers",
                    "The server emulator has no running game servers.");
                dialogPtr->close();
                return;
            }
            if (availableServers.size() > 1) {
                QMessageBox::warning(dialogPtr, "Not Implemented Yet",
                    "The server emulator currently has more than one game server running at a time. Right now there is no implemented behavior for picking from a selection of game servers, so you'll just be joining the first one on the list.");
            }
            dialogPtr->close();
            LaunchEngine(availableServers.at(0));
        });
    });
}

int main(int argc, char **argv) {
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(shared_resources); // you must do this or else the compiler will optimize it out of the code.
    Application app(argc, argv);
    gApp = &app;
    return app.Run();
}
