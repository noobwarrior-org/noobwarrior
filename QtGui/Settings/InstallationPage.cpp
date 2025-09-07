// === noobWarrior ===
// File: InstallationPage.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#include "InstallationPage.h"
#include "../Application.h"

#include <NoobWarrior/RobloxClient.h>

#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QDateTime>

#include <curl/curl.h>

using namespace NoobWarrior;

static std::map<ClientType, QString> sIcons = {
    { ClientType::Client, QString(":/images/client.png") },
    { ClientType::Server, QString(":/images/server.png") },
    { ClientType::Studio, QString(":/images/studio.png") }
};

InstallationPage::InstallationPage(QWidget *parent) : SettingsPage(parent),
    HorizontalLayout(nullptr),
    ListWidget(nullptr),
    StackedWidget(nullptr),
    CannotConnectLabel(nullptr)
{
    Init();
    InitWidgets();
}

void InstallationPage::InitWidgets() {
    HorizontalLayout = new QHBoxLayout();

    ListWidget = new QListWidget();
    ListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ListWidget->setFixedWidth(128);

    StackedWidget = new QStackedWidget();

    for (int i = 0; i <= ClientTypeCount; i++) {
        auto clientType = static_cast<ClientType>(i);
        auto clientTypeItem = new QListWidgetItem(QIcon(sIcons[clientType]), ClientTypeAsTranslatableString(clientType), ListWidget);
        QFont font = clientTypeItem->font();
        font.setPointSize(12);
        clientTypeItem->setFont(font);

        auto clientView = new QTreeView(StackedWidget);
        clientView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        clientView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        auto clientModel = new QStandardItemModel(clientView);
        clientModel->setColumnCount(6);
        clientModel->setHorizontalHeaderLabels({"", "Latest", "Installed", "Version", "Hash", "Date"});
        clientView->setModel(clientModel);
        clientView->setColumnHidden(0, true); // for some odd reason, the first column cannot be reordered. so we just make it blank and hide it. good job!

        ClientVersionViewMap.emplace(clientType, clientView);
        ClientVersionModelMap.emplace(clientType, clientModel);
        StackedWidget->addWidget(clientView);

        connect(ListWidget, &QListWidget::currentItemChanged, [this, i, clientTypeItem](QListWidgetItem *current, QListWidgetItem *previous) {
            if (current == clientTypeItem)
                StackedWidget->setCurrentIndex(i);
        });
    }

    HorizontalLayout->addWidget(ListWidget);
    HorizontalLayout->addWidget(StackedWidget);

    IndexMessageLabel = new QLabel("");
    IndexMessageLabel->setWordWrap(true);
    IndexMessageLabel->setStyleSheet("QLabel { color: orange; }");
    IndexMessageLabel->setVisible(false);

    Layout->addWidget(IndexMessageLabel);
    Layout->addLayout(HorizontalLayout);

    CannotConnectLabel = new QLabel("");
    CannotConnectLabel->setWordWrap(true);
    CannotConnectLabel->setVisible(false);
    Layout->addWidget(CannotConnectLabel);
}

void InstallationPage::Refresh() {
    nlohmann::json index;
    int res = gApp->GetCore()->RetrieveIndex(index);
    if (res != CURLE_OK) {
        auto url = gApp->GetCore()->GetConfig()->GetKeyValue<std::string>("internet.index");
        QString errMsg;
        switch (res) {
            default: errMsg = QString("Could not connect to server for file \"\"").arg(QString::fromStdString(url.value())); break;
            case -1: errMsg = "Could not connect to server to retrieve clients; no URL is set!"; break;
        }
        CannotConnectLabel->setText(errMsg);
        CannotConnectLabel->setVisible(true);
        return;
    }

#define REQUIRE(key) if (!index.contains(key)) { QMessageBox::critical(this, "Cannot Load Index", QString("This JSON is malformed since it does not contain a \"%1\" object.").arg(key)); return; }
    REQUIRE("Version")
    REQUIRE("Roblox")
#undef REQUIRE

    if (index.contains("Message") && !index["Message"].get<std::string>().empty()) {
        IndexMessageLabel->setText(QString::fromStdString(index["Message"].get<std::string>()));
        IndexMessageLabel->setVisible(true);
    }

    for (int i = 0; i <= ClientTypeCount; i++) {
        auto clientType = static_cast<ClientType>(i);
        const char* clientTypeStr = ClientTypeAsTranslatableString(clientType);
        QStandardItemModel *clientVersionModel = ClientVersionModelMap.at(clientType);
        clientVersionModel->removeRows(0, clientVersionModel->rowCount());
        clientVersionModel->setRowCount(0);

        if (!index["Roblox"].contains(clientTypeStr)) {
            QMessageBox::critical(this, "Error", QString("The index does not contain \"%1\" within the list.").arg(clientTypeStr));
            continue;
        }

        for (auto &clientInfo : index["Roblox"][clientTypeStr]) {
            std::string hash = clientInfo.at("Hash").get<std::string>();
            std::string version = clientInfo.at("Version").get<std::string>();
            std::string date = clientInfo.at("Date").get<std::string>();

            auto dateTime = QDateTime::fromString(QString::fromStdString(date), Qt::ISODate);

            RobloxClient client = {
                .Type = clientType,
                .Hash = hash,
                .Version = version
            };

            QList<QStandardItem*> clientRow;
            clientRow
                << new QStandardItem("")
                << new QStandardItem(QIcon(":/images/silk/star.png"), "")
                << new QStandardItem(QIcon(gApp->GetCore()->IsClientInstalled(client) ? ":/images/silk/tick.png" : ":/images/silk/cross.png"), "")
                << new QStandardItem(QString::fromStdString(version))
                << new QStandardItem(QString::fromStdString(hash))
                << new QStandardItem(dateTime.toString("ddd MMMM d yyyy h:mm:ss AP"));
            clientVersionModel->appendRow(clientRow);
        }

        QTreeView *clientVersionView = ClientVersionViewMap.at(clientType);
        for (int i = 0; i < clientVersionModel->columnCount(); i++) {
            clientVersionView->resizeColumnToContents(i);
        }
    }
}

const QString InstallationPage::GetTitle() {
    return "Roblox Installations";
}

const QString InstallationPage::GetDescription() {
    return "All of the versions of Roblox that noobWarrior can install (or has installed) on your computer. Please note that you cannot just copy over an existing installation of Roblox, they have to be manually patched by us.";
}

const QIcon InstallationPage::GetIcon() {
    return QIcon(":/images/roblox_folder.png");
}
