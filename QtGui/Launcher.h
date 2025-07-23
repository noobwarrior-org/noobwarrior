// === noobWarrior ===
// File: Launcher.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description:
#pragma once

#include "DatabaseEditor/DatabaseEditor.h"
#include "AssetDownloader.h"
#include "Settings/SettingsDialog.h"
#include "Dialog/AboutDialog.h"

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
    private:
        QVBoxLayout *Layout;
    };
}