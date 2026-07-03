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
// File: Application.h
// Started by: Hattozo
// Started on: 5/18/2025
// Description:
#pragma once
#include <NoobWarrior/NoobWarrior.h>
#include "Launcher.h"

#include <QApplication>
#include <QSystemTrayIcon>

namespace NoobWarrior {
class Application : public QApplication {
public:
    enum class Theme {
        System,
        Darcula
    };

    Application(int &argc, char **argv);
    int Run();
    Core *GetCore();
    bool CheckConfigResponse(RegistryResponse res, const QString &errStr);

    // GUI versions for downloading engines
    void DownloadAndInstallWine(std::function<void(bool)> callback);
    void DownloadAndInstallEngine(const Engine &engine, std::function<void(bool)> callback);
    void LaunchEngine(EngineStartParameters params);
    void ConnectToServer(const std::string &ip, uint16_t port);
    void ShowSystemNotification(const QString &title, const QString &message);
private:
    // Connect helpers: ConnectToServer fetches the host's auth-info, PromptAndConnect shows the login
    // dialog when needed, DoConnect runs the actual connect with an optional session token.
    void PromptAndConnect(const std::string &ip, uint16_t port, bool authEnabled, bool passwordBased,
                          bool allowGuests, const QString &title, const QString &tagline);
    void DoConnect(const std::string &ip, uint16_t port, const std::string &sessionToken);

    Init mInit {};
    Core *mCore;
    Launcher *mLauncher;
    QSystemTrayIcon *mTrayIcon;
    QMenu *mTrayMenu;
};
extern Application *gApp;
}