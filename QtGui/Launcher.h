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