// === noobWarrior ===
// File: MasterServerWindow.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Window that contains features that the master server can present
#pragma once
#include "MasterServerSidebar.h"
#include "ServerListWidget.h"

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>

namespace NoobWarrior {
class MasterServerWindow : public QMainWindow {
    Q_OBJECT
public:
    MasterServerWindow(QWidget* parent = nullptr);
private:
    void InitWidgets();

    MasterServerSidebar* mSidebar;
    ServerListWidget* mServerList;
    QTabWidget* ServersTab;
};
}