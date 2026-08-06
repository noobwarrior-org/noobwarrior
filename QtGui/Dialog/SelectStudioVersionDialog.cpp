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
// File: SelectStudioVersionDialog.cpp
// Started by: Hattozo
// Started on: 4/21/2026
// Description:
#include "SelectStudioVersionDialog.h"
#include "Application.h"

#include <NoobWarrior/NoobWarrior.h>

#include <QListWidget>
#include <QMessageBox>

using namespace NoobWarrior;

SelectStudioVersionDialog::SelectStudioVersionDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Select Studio Version");
    InitWidgets();
}

void SelectStudioVersionDialog::InitWidgets() {
    mLayout = new QVBoxLayout(this);

    mLabel = new QLabel("Choose the version of Studio that you would like to open.");
    mLabel->setWordWrap(true);
    mLayout->addWidget(mLabel);

    mListWidget = new QListWidget();
    mListWidget->setAutoFillBackground(true);

    for (auto &engine : gApp->GetCore()->GetInstalledEngines()) {
        if (engine.Side == EngineSide::Studio) {
            QString label = engine.Version.empty()
                ? QString("Unknown version (%1)").arg(QString::fromStdString(engine.Hash))
                : QString::fromStdString(engine.Version);
            auto *item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, QVariant::fromValue(engine));
            mListWidget->addItem(item);
        }
    }
    mLayout->addWidget(mListWidget);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(mButtonBox, &QDialogButtonBox::accepted, [this]() {
        if (!mListWidget->selectedItems().empty()) {
            auto *item = mListWidget->selectedItems().at(0);
            auto engine = item->data(Qt::UserRole).value<Engine>();
            gApp->LaunchEngine({
                .Engine = engine,
                .LaunchSide = EngineSide::Studio
            });
        } else {
            QMessageBox::critical(this, "Error", "You need to select a version!");
            return;
        }
        close();
    });
    connect(mButtonBox, &QDialogButtonBox::rejected, [this]() {
        close();
    });
    mLayout->addWidget(mButtonBox);
}
