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
// File: HostServerDialog.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Dialog that allows for starting a Roblox game server
#pragma once
#include <NoobWarrior/EmuDb/EmuDb.h>

#include "Sdk/EmuDbListWidget.h"
#include "Sdk/Item/ItemListWidget.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace NoobWarrior {
class HostServerDialog : public QDialog {
    Q_OBJECT
public:
    HostServerDialog(QWidget* parent = nullptr);
private:
    void InitWidgets();
    QVBoxLayout* mLayout;
    QHBoxLayout* mMainLayout;

    EmuDbListWidget* mDbListWidget;
    QTreeWidget* mTreeWidget;

    QDialogButtonBox* mButtonBox;
    QPushButton* mStartServer;
    QPushButton* mCloseButton;
};
}