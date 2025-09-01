// === noobWarrior ===
// File: SettingsDialog.cpp
// Started by: Hattozo
// Started on: 3/22/2025
// Description: Settings menu to configure user preferences
#include "SettingsDialog.h"
#include "../Application.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFrame>
#include <QLineEdit>

#include "DatabasePage.h"
#include "GeneralPage.h"
#include "HttpServerPage.h"
#include "InstallationPage.h"
#include "AccountPage.h"
#include "SettingsPage.h"

// #define PAGE(name, icon) { auto *page = new QFrame(tab); auto *layout = new QVBoxLayout(page); page->setLayout(layout); tab->addTab(page, QIcon(icon), name); }
// #define INPUT(name, configProp, category, pageName) { QWidget *page = tab->findChild<QWidget*>(pageName); auto *w = new QLineEdit(page); w->setText(QString::fromStdString(configProp)); page->layout()->addWidget(w); }

using namespace NoobWarrior;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("noobWarrior Settings");
    InitWidgets();
}

void SettingsDialog::InitWidgets() {
    auto *mainLayout = new QHBoxLayout(this);

    ListWidget = new QListWidget();
    ListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ListWidget->setFixedWidth(160);

    StackedWidget = new QStackedWidget();

    InitPages();

    // Pages.push_back()

    // PAGE("General", ":/images/silk/cog.png")
    // PAGE("Internet", ":/images/silk/world.png")

    // INPUT("Asset Delivery Api", gApp->GetCore()->GetConfig()->Api_AssetDownload, "Roblox API URLs", "Internet")

    mainLayout->addWidget(ListWidget);
    mainLayout->addWidget(StackedWidget);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SettingsDialog::InitPages() {
    AddPage(new GeneralPage());
    AddPage(new DatabasePage());
    AddPage(new HttpServerPage());
    AddPage(new InstallationPage());
    AddPage(new AccountPage());
}

void SettingsDialog::AddPage(SettingsPage *page) {
    auto button = new QListWidgetItem(page->GetIcon(), page->GetTitle());
    int index = StackedWidget->addWidget(page);
    ListWidget->addItem(button);
    connect(ListWidget, &QListWidget::currentItemChanged, [this, button, index](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current == button)
            StackedWidget->setCurrentIndex(index);

        // Page specific code for anything that needs to be lazily loaded.
        auto installationPage = dynamic_cast<InstallationPage*>(StackedWidget->currentWidget());
        if (installationPage != nullptr) {
            installationPage->Refresh();
        }
    });
}
