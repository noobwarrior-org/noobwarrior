// === noobWarrior ===
// File: MasterServerSidebar.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Widget that contains a list of master servers
#include "MasterServerSidebar.h"
#include "../Application.h"

#include <NoobWarrior/NetClient.h>

using namespace NoobWarrior;

MasterServerSidebar::MasterServerSidebar(QWidget* parent) : QDockWidget(parent) {
    setWindowTitle("Sidebar");
    InitWidgets();
}

void MasterServerSidebar::InitWidgets() {
    mView = new QTreeView(this);
    mView->setHeaderHidden(true);

    mModel = new QStandardItemModel(this);

    auto *favoritesRow = new QStandardItem(QIcon(":/images/silk/house.png"), QString::fromStdString("Favorites & LAN Servers"));
    auto *recentsRow = new QStandardItem(QIcon(":/images/silk/time.png"), QString::fromStdString("Recently Played Servers"));

    mModel->invisibleRootItem()->appendRow(favoritesRow);
    mModel->invisibleRootItem()->appendRow(recentsRow);
    
    std::optional<nlohmann::json> servers = gApp->GetCore()->GetConfig()->GetKeyValue<nlohmann::json>("gui.master_servers");
    if (servers.has_value()) {
        for (auto &server : servers.value()) {
            NetClient client;
            client.Request(server["url"].get<std::string>() + "/autodiscover");
            client.OnWriteToMemoryFinished([](std::vector<unsigned char> &data) -> void {

            });
            std::string name = server.contains("name") ? server["name"].get<std::string>() : "Loading...";
            QStandardItem *serverRow = new QStandardItem(QIcon(":/images/silk/server.png"), QString::fromStdString(name));

            QStandardItem *serversRow = new QStandardItem(QIcon(":/images/silk/controller.png"), "Servers");
            QStandardItem *newsRow = new QStandardItem(QIcon(":/images/silk/newspaper.png"), "News");
            QStandardItem *accountRow = new QStandardItem(QIcon(":/images/silk/report_user.png"), "Account");
            QStandardItem *avatarRow = new QStandardItem(QIcon(":/images/silk/user_suit.png"), "Avatar");
            QStandardItem *friendsRow = new QStandardItem(QIcon(":/images/silk/folder_user.png"), "Friends");

            serverRow->appendRow(serversRow);
            serverRow->appendRow(newsRow);
            serverRow->appendRow(accountRow);
            accountRow->appendRow(avatarRow);
            accountRow->appendRow(friendsRow);

            mModel->invisibleRootItem()->appendRow(serverRow);
        }
    }

    mView->setModel(mModel);
    mView->expandAll();
    mView->show();

    setWidget(mView);
}