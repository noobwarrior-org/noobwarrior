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
// File: InstallationPage.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <NoobWarrior/Engine.h>

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QTreeView>
#include <QStandardItemModel>

#include <nlohmann/json.hpp>

namespace NoobWarrior {
class InstallationPage : public SettingsPage {
public:
    InstallationPage(QWidget *parent = nullptr);
    void InitWidgets();
    void Refresh();

    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
private:
    QHBoxLayout *HorizontalLayout;
    QListWidget *ListWidget;
    QStackedWidget *StackedWidget;

    QLabel *IndexMessageLabel;
    QLabel *CannotConnectLabel;

    std::map<EngineSide, QTreeView*> EngineVersionViewMap;
    std::map<EngineSide, QStandardItemModel*> EngineVersionModelMap;
};
}
