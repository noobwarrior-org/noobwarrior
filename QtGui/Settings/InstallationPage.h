// === noobWarrior ===
// File: InstallationPage.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <NoobWarrior/RobloxClient.h>

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QTreeView>
#include <QStandardItemModel>

#include <nlohmann/json.hpp>

namespace NoobWarrior {
class InstallationPage : public SettingsPage {
public:
    enum class NetResponse {
        Failed,
        Success,
        NoUrl
    };

    InstallationPage(QWidget *parent = nullptr);
    void InitWidgets();
    void Refresh();

    bool IsRefreshDirty();

    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
private:
    NetResponse RequestIndex(nlohmann::json &json);

    bool mDirtyRefresh;

    QHBoxLayout *HorizontalLayout;
    QListWidget *ListWidget;
    QStackedWidget *StackedWidget;

    QLabel *CannotConnectLabel;

    std::map<ClientType, QTreeView*> ClientVersionViewMap;
    std::map<ClientType, QStandardItemModel*> ClientVersionModelMap;
};
}
