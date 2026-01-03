// === noobWarrior ===
// File: MasterServerWindow.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Window that contains features that the master server can present
#include "MasterServerWindow.h"

using namespace NoobWarrior;

MasterServerWindow::MasterServerWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Servers");
    InitWidgets();
}

void MasterServerWindow::InitWidgets() {
    mServerList = new ServerListWidget(this);

    mSidebar = new MasterServerSidebar(this);
    mSidebar->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, mSidebar);
}