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
// cpr must precede anything that pulls in evhttp.h, whose HTTP_* macros break cpr's status_codes.h.
#include <cpr/cpr.h>

#include "Application.h"
#include "LoadingDialog.h"
#include "OnlineWindow/ServerLoginDialog.h"
#include "OnlineWindow/MasterLoginDialog.h"
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
#include <QSystemTrayIcon>
#include <QSharedMemory>

#include <curl/curl.h>
#include <event.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <optional>
#include <thread>

#define USE_CUSTOM_STYLE 1
#define SHOW_DISCLAIMER 0

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

    int ret = 1;

    mCore = new Core(mInit);

    // Only one instance may run at a time, unless allow_multiple_instances is set
    QSharedMemory sharedMemory("noobWarrior");
    bool allowMultiple = mCore->GetRegistry() != nullptr &&
        mCore->GetRegistry()->GetKeyValue<bool>("allow_multiple_instances").value_or(false);
    if (!allowMultiple && !sharedMemory.create(1)) {
        QMessageBox::critical(nullptr, "Error", "Another instance is already running!");
        return 0;
    }

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
    
#if SHOW_DISCLAIMER
    QString title = "Disclaimer";
    QString text =
        "This program is just a proof of concept at the moment. It's not finished. "
        "You'll inevitably run into missing functionality or things that are just straight up broken. "
        "You should only be using this program to see what you think of it, and to test it for bugs and other oddities."
        "\n\nBy clicking Yes, you agree to the statements made above.";

    QMessageBox msg;
#if !defined(Q_OS_MACOS)
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setIcon(QMessageBox::Information);
#else
    msg.setText(title);
    msg.setInformativeText(text);
#endif
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg.setDefaultButton(QMessageBox::No);
    int res = msg.exec();
    if (res != QMessageBox::Yes)
        goto cleanup;
#endif
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

    connect(dialog, &QWidget::destroyed, []() {
    });

    dialog->show();

    QPointer<LoadingDialog> dialogPtr(dialog);

    mCore->DownloadAndInstallEngine(client, []() {
        
    });
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
            case EngineLaunchResponse::WineMissing: errMsg = "A Wine installation could not be found on your system. Please install it. If you have installed it, go to the Settings menu and configure your Wine path."; break;
            case EngineLaunchResponse::FailedToLoadPlace: errMsg = "Failed to write to the server.rbxl file."; break;
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
    // Ask the host whether it requires auth before doing anything else.
    std::string url = "https://" + ip + ":" + std::to_string(port) + "/emu/v1/auth-info";
    QPointer<Application> self(this);
    std::thread([self, ip, port, url]() {
        bool authEnabled = false, passwordBased = true, allowGuests = false;
        std::string title, tagline, authType = "master", authMasterUrl;
        cpr::Response res = cpr::Get(cpr::Url{url}, cpr::VerifySsl{false},
                                     cpr::Timeout{std::chrono::milliseconds(5000)});
        if (res.error.code == cpr::ErrorCode::OK && res.status_code == 200) {
            nlohmann::json j = nlohmann::json::parse(res.text, nullptr, false);
            if (!j.is_discarded()) {
                authEnabled   = j.value("authEnabled", false);
                passwordBased = j.value("passwordBased", true);
                allowGuests   = j.value("allowGuests", false);
                authType      = j.value("authType", std::string{"master"});
                authMasterUrl = j.value("authMasterUrl", std::string{});
                nlohmann::json b = j.value("branding", nlohmann::json::object());
                title   = b.value("title", std::string{});
                tagline = b.value("tagline", std::string{});
            }
        }
        QTimer::singleShot(0, qApp, [self, ip, port, authEnabled, passwordBased, allowGuests, authType, authMasterUrl, title, tagline]() {
            if (!self) return;
            self->PromptAndConnect(ip, port, authEnabled, passwordBased, allowGuests,
                                   QString::fromStdString(authType), QString::fromStdString(authMasterUrl),
                                   QString::fromStdString(title), QString::fromStdString(tagline));
        });
    }).detach();
}

void Application::PromptAndConnect(const std::string &ip, uint16_t port, bool authEnabled,
                                   bool passwordBased, bool allowGuests, const QString &authType,
                                   const QString &authMasterUrl, const QString &title,
                                   const QString &tagline) {
    if (!authEnabled) {
        DoConnect(ip, port, "");
        return;
    }

    // Slave mode: identity comes from a master server. Reuse the active master account if we have one,
    // otherwise prompt the player to sign in to their master (or play as guest).
    if (authType == "slave") {
        if (Account *active = mCore->GetMasterKeychain()->GetActiveAccount();
            active != nullptr && !active->Token.empty() && !active->Url.empty()) {
            ConnectWithMaster(ip, port, QString::fromStdString(active->Url),
                              QString::fromStdString(active->Token), authMasterUrl);
            return;
        }

        MasterLoginDialog dlg(nullptr, title, tagline, authMasterUrl, allowGuests);
        if (dlg.exec() != QDialog::Accepted)
            return; // cancelled
        if (dlg.SelectedMode() == MasterLoginDialog::Mode::Guest) {
            DoConnect(ip, port, "");
            return;
        }

        Core* core = mCore;
        QPointer<Application> self(this);
        std::string masterUrl = dlg.MasterUrl().toStdString();
        std::string username = dlg.Username().toStdString();
        std::string password = dlg.Password().toStdString();
        std::string target = authMasterUrl.toStdString();
        std::thread([self, core, ip, port, masterUrl, username, password, target]() {
            bool ok = core->LoginToMaster(masterUrl, username, password);
            // LoginToMaster stores the fresh session on the now-active master account.
            Account *acc = ok ? core->GetMasterKeychain()->GetActiveAccount() : nullptr;
            std::string token = acc != nullptr ? acc->Token : "";
            QTimer::singleShot(0, qApp, [self, ip, port, masterUrl, token, target]() {
                if (!self) return;
                if (token.empty()) {
                    QMessageBox::critical(nullptr, "Sign in failed", "Your master server rejected those credentials.");
                    return;
                }
                self->ConnectWithMaster(ip, port, QString::fromStdString(masterUrl), QString::fromStdString(token),
                                        QString::fromStdString(target));
            });
        }).detach();
        return;
    }

    // Master mode: reuse a cached login for this host if it's still valid, else prompt.
    std::string cachedToken = mCore->GetCachedRemoteHostToken(ip, port);
    if (!cachedToken.empty()) {
        Core* core = mCore;
        QPointer<Application> self(this);
        std::thread([self, core, ip, port, cachedToken, passwordBased, allowGuests, title, tagline]() {
            bool valid = core->ValidateRemoteHostSession(ip, port, cachedToken);
            if (!valid)
                core->ForgetRemoteHostLogin(ip, port); // stale: drop it so we don't loop on a dead token
            QTimer::singleShot(0, qApp, [self, ip, port, valid, cachedToken, passwordBased, allowGuests, title, tagline]() {
                if (!self) return;
                if (valid)
                    self->DoConnect(ip, port, cachedToken);
                else
                    self->PromptMasterLogin(ip, port, passwordBased, allowGuests, title, tagline);
            });
        }).detach();
        return;
    }
    PromptMasterLogin(ip, port, passwordBased, allowGuests, title, tagline);
}

void Application::PromptMasterLogin(const std::string &ip, uint16_t port, bool passwordBased,
                                    bool allowGuests, const QString &title, const QString &tagline) {
    // Log in directly against the host (master mode). On success LoginToRemoteHost caches the session.
    ServerLoginDialog dlg(nullptr, title, tagline, passwordBased, allowGuests);
    if (dlg.exec() != QDialog::Accepted)
        return; // cancelled
    if (dlg.SelectedMode() == ServerLoginDialog::Mode::Guest) {
        DoConnect(ip, port, "");
        return;
    }

    Core* core = mCore;
    QPointer<Application> self(this);
    std::string username = dlg.Username().toStdString();
    std::string password = dlg.Password().toStdString();
    std::thread([self, core, ip, port, username, password]() {
        std::optional<std::string> token = core->LoginToRemoteHost(ip, port, username, password);
        QTimer::singleShot(0, qApp, [self, ip, port, token]() {
            if (!self) return;
            if (!token) {
                QMessageBox::critical(nullptr, "Login Failed", "The server rejected those credentials.");
                return;
            }
            self->DoConnect(ip, port, *token);
        });
    }).detach();
}

void Application::ConnectWithMaster(const std::string &ip, uint16_t port, const QString &masterUrl,
                                    const QString &sessionToken, const QString &targetMasterUrl) {
    Core* core = mCore;
    QPointer<Application> self(this);
    std::string master = masterUrl.toStdString();
    std::string token = sessionToken.toStdString();
    std::string target = targetMasterUrl.toStdString();
    std::thread([self, core, ip, port, master, token, target]() {
        std::string error;
        std::optional<std::string> voucher = core->MintJoinVoucher(master, token, target, &error);
        QTimer::singleShot(0, qApp, [self, ip, port, voucher, error]() {
            if (!self) return;
            if (!voucher) {
                QMessageBox::critical(nullptr, "Join failed",
                    QString("Could not obtain a join voucher:\n\n%1").arg(QString::fromStdString(error)));
                return;
            }
            self->DoConnect(ip, port, *voucher);
        });
    }).detach();
}

void Application::DoConnect(const std::string &ip, uint16_t port, const std::string &sessionToken) {
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
    }, sessionToken);
}

void Application::ShowSystemNotification(const QString &title, const QString &message) {
    if (mTrayIcon != nullptr && QSystemTrayIcon::supportsMessages())
        mTrayIcon->showMessage(title, message, QSystemTrayIcon::Information, 8000);
}

int main(int argc, char **argv) {
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(shared_resources); // you must do this or else the compiler will optimize it out of the code.
    Application app(argc, argv);
    gApp = &app;
    return app.Run();
}
