// === noobWarrior ===
// File: PluginDialog.cpp
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#include "PluginDialog.h"

#include "../Application.h"

using namespace NoobWarrior;

PluginDialog::PluginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Plugins");
    InitWidgets();
}

void PluginDialog::InitWidgets() {
    mLayout = new QVBoxLayout(this);
    mView = new QTreeView();
    mModel = new QStandardItemModel(mView);
    mLayout->addWidget(mView);

    mModel->setColumnCount(5);
    mModel->setHorizontalHeaderLabels({"", "Enabled", "Icon", "Info", "Description"});
    mView->setModel(mModel);
    mView->setColumnHidden(0, true);

    for (const Plugin::Properties &props : gApp->GetCore()->GetPluginManager()->GetAllPluginProperties()) {
        std::string authors;
        for (std::string author : props.Authors)
            authors.append(author + ", ");

        QList<QStandardItem*> pluginRow;
        pluginRow
            << new QStandardItem("")
            << new QStandardItem("") // Checkmark box for enabling it
            << new QStandardItem(QIcon(), "")
            << new QStandardItem(QString("%1\n%2\n%3").arg(props.Title, props.Version, authors))
            << new QStandardItem(QString::fromStdString(props.Description));
        mModel->appendRow(pluginRow);
    }
}