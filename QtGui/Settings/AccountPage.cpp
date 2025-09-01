// === noobWarrior ===
// File: AccountPage.cpp
// Started by: Hattozo
// Started on: 8/30/2025
// Description:
#include "AccountPage.h"
#include "../Application.h"
#include "../Dialog/AuthTokenDialog.h"

using namespace NoobWarrior;

AccountPage::AccountPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void AccountPage::InitWidgets() {
    auto addTokenButton = new QPushButton("Add Account Using Token", this);
    connect(addTokenButton, &QPushButton::clicked, [this]() {
        AuthTokenDialog dialog(this);
        dialog.exec();
    });
    Layout->addWidget(addTokenButton);

    ListView = new QTreeView(this);
    ListView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    Layout->addWidget(ListView);

    ListModel = new QStandardItemModel(ListView);
    ListModel->setColumnCount(6);
    ListModel->setHorizontalHeaderLabels({"", "Active", "Id", "Name", "Expiration Date", "Expired?"});
    ListView->setModel(ListModel);
    ListView->setColumnHidden(0, true); // for some odd reason, the first column cannot be reordered. so we just make it blank and hide it. good job!

    Refresh();
}

void AccountPage::Refresh() {
    Auth *auth = gApp->GetCore()->GetAuth();
    std::vector<Account> accounts = auth->GetAccounts();
    for (auto &acc : accounts) {
        QList<QStandardItem*> accRow;
            accRow
                << new QStandardItem()
                << new QStandardItem(QIcon(auth->GetActiveAccount() == &acc ? ":/images/silk/tick.png" : ""), "")
                << new QStandardItem(acc.Id > -1 ? QString::number(acc.Id) : "N/A")
                << new QStandardItem(!acc.Name.empty() ? QString::fromStdString(acc.Name) : "N/A")
                << new QStandardItem(acc.ExpireTimestamp > -1 ? "idk" : "N/A")
                << new QStandardItem(Auth::HasAccountExpired(acc) ? "Yes" : "No");
            ListModel->appendRow(accRow);
    }
}

const QString AccountPage::GetTitle() {
    return "Roblox Accounts";
}

const QString AccountPage::GetDescription() {
    return "All of the Roblox accounts that noobWarrior is able to access. This will give full access to your Roblox account when added, so make sure your copy of noobWarrior is not tampered with.";
}

const QIcon AccountPage::GetIcon() {
    return QIcon(":/images/silk/folder_user.png");
}
