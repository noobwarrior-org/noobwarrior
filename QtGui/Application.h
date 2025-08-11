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
    void DownloadAndInstallClient(const RobloxClient &client);
    void LaunchClient(const RobloxClient &client);
private:
    Init mInit {};
    Core *mCore;
    Launcher *mLauncher;
};
extern Application *gApp;
}