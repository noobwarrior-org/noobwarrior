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
// File: SettingsDialog.h
// Started by: Hattozo
// Started on: 3/22/2025
// Description:
#pragma once
#include "SettingsPage.h"

#include <NoobWarrior/Config.h>
#include <QDialog>
#include <QFrame>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDialogButtonBox>

namespace NoobWarrior {
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = nullptr);
    void InitWidgets();
    void InitPages();
    void AddPage(SettingsPage *page);
private:
    QListWidget *ListWidget;
    QStackedWidget *StackedWidget;
    QDialogButtonBox *ButtonBox;
    std::vector<SettingsPage*> Pages;
};
}
