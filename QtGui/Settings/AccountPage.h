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
