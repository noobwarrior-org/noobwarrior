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

namespace NoobWarrior {
class Application : public QApplication {
public:
    Application(int &argc, char **argv);
    int Run();
    Core *GetCore();
    bool CheckConfigResponse(ConfigResponse res, const QString &errStr);

    // GUI versions for downloading clients
    void DownloadAndInstallClient(const RobloxClient &client, std::function<void(bool)> callback);
    void LaunchClient(const RobloxClient &client);
private:
    Init mInit {};
    Core *mCore;
    Launcher *mLauncher;
};
extern Application *gApp;
}