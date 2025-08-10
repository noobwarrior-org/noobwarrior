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

#include <thread>
#include <future>

#include <curl/curl.h>

using namespace NoobWarrior;

InstallationPage::InstallationPage(QWidget *parent) : SettingsPage(parent),
    mDirtyRefresh(true),
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
        auto clientTypeItem = new QListWidgetItem(ClientTypeAsTranslatableString(clientType), ListWidget);
        QFont font = clientTypeItem->font();
        font.setPointSize(12);
        clientTypeItem->setFont(font);

        auto clientView = new QTreeView(StackedWidget);
        clientView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto clientModel = new QStandardItemModel(clientView);
        clientModel->setColumnCount(6);
        clientModel->setHorizontalHeaderLabels({"", "Latest", "Installed", "Version", "Hash", "Date"});
        clientView->setModel(clientModel);

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

    Layout->addLayout(HorizontalLayout);

    CannotConnectLabel = new QLabel(QString("Could not connect to server for file \"\""));
    CannotConnectLabel->setVisible(false);
    Layout->addWidget(CannotConnectLabel);
}

void InstallationPage::Refresh() {
    nlohmann::json index;
    NetResponse res = RequestIndex(index);

    // We've tried our luck. It's not dirty anymore. If it fails beyond this point, the user will need to manually refresh.
    mDirtyRefresh = false;

    if (res != NetResponse::Success) {
        auto url = gApp->GetCore()->GetConfig()->GetKeyValue<std::string>("internet.index");
        QString errMsg;
        switch (res) {
            default: errMsg = QString("Could not connect to server for file \"\"").arg(url.value()); break;
            case NetResponse::NoUrl: errMsg = "Could not connect to server to retrieve clients; no URL is set!"; break;
        }
        CannotConnectLabel->setText(errMsg);
        CannotConnectLabel->setVisible(true);
        return;
    }

#define REQUIRE(key) if (!index.contains(key)) { QMessageBox::critical(this, "Cannot Load Index", QString("This JSON is malformed since it does not contain a \"%1\" object.").arg(key)); return; }
    REQUIRE("Meta")
    REQUIRE("Roblox")
#undef REQUIRE

    for (int i = 0; i <= ClientTypeCount; i++) {
        auto clientType = static_cast<ClientType>(i);
        const char* clientTypeStr = ClientTypeAsTranslatableString(clientType);
        QStandardItemModel *clientVersionModel = ClientVersionModelMap.at(clientType);

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

bool InstallationPage::IsRefreshDirty() {
    return mDirtyRefresh;
}

static size_t WriteToString(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

InstallationPage::NetResponse InstallationPage::RequestIndex(nlohmann::json &json) {
    auto url = gApp->GetCore()->GetConfig()->GetKeyValue<const char*>("internet.index");

    if (!url.has_value())
        return NetResponse::NoUrl;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.value());

    std::string jsonStr;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonStr);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
        return NetResponse::Failed;

    json = nlohmann::json::parse(jsonStr);
    return NetResponse::Success;
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
