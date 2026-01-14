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
// File: Launcher.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description:
#pragma once
#include "DatabaseEditor/DatabaseEditor.h"
#include "Dialog/AssetDownloaderDialog.h"
#include "Settings/SettingsDialog.h"
#include "Dialog/AboutDialog.h"
#include "Dialog/DatabaseDialog.h"
#include "Dialog/PluginDialog.h"
#include "ServerHost/HostServerDialog.h"
#include "MasterServer/MasterServerWindow.h"

#include <QDialog>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui { class Launcher; }
QT_END_NAMESPACE

namespace NoobWarrior {
    class Launcher : public QDialog {
        Q_OBJECT

    public:
        Launcher(QWidget *parent = nullptr);
        ~Launcher();

        AboutDialog *mAboutDialog;
        SettingsDialog *mSettings;
        DatabaseEditor *mDatabaseEditor;
        AssetDownloader *mAssetDownload;
        HostServerDialog *mHostServerDialog;
        MasterServerWindow *mMasterServerWindow;
        DatabaseDialog *mDatabaseDialog;
        PluginDialog *mPluginDialog;
    protected:
        void paintEvent(QPaintEvent *event) override;
    private:
        QVBoxLayout*    Layout;
        QLabel*         AuthenticationStatusLabel;
        QLabel*         ServerEmulatorStatusLabel;
    };
}