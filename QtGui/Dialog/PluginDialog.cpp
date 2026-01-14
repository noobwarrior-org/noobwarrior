/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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