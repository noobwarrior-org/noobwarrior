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

#define PAGE(name, icon) { auto *page = new QFrame(tab); auto *layout = new QVBoxLayout(page); page->setLayout(layout); tab->addTab(page, QIcon(icon), name); }
#define INPUT(name, configProp, category, pageName) { QWidget *page = tab->findChild<QWidget*>(pageName); auto *w = new QLineEdit(page); w->setText(QString::fromStdString(configProp)); page->layout()->addWidget(w); }

using namespace NoobWarrior;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("noobWarrior Settings");
    auto *mainLayout = new QVBoxLayout(this);
    auto *tab = new QTabWidget();

    QFrame *general;
    QFrame *internet;

    PAGE("General", ":/images/silk/cog.png")
    PAGE("Internet", ":/images/silk/world.png")

    INPUT("Asset Delivery Api", gApp->GetCore()->GetConfig()->Api_AssetDownload, "Roblox API URLs", "Internet")

    mainLayout->addWidget(tab);
}