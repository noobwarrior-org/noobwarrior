// === noobWarrior ===
// File: MasterServerSidebar.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of master servers
#include "MasterServerSidebar.h"

using namespace NoobWarrior;

MasterServerSidebar::MasterServerSidebar(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle("Master Servers");
    InitWidgets();
}

void MasterServerSidebar::InitWidgets() {
    mView = new QTreeView(this);
    setWidget(mView);
}