// === noobWarrior ===
// File: AccountPage.h
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>

namespace NoobWarrior {
class AccountPage : public SettingsPage {
public:
    AccountPage(QWidget *parent = nullptr);
    void InitWidgets();
    void Refresh();
    
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
private:
    QTreeView *ListView;
    QStandardItemModel *ListModel;
};
}
