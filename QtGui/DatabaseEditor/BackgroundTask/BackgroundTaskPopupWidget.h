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
// File: BackgroundTaskPopupWidget.h
// Started by: Hattozo
// Started on: 12/28/2025
// Description: The full dialog that pops up when you click on that little progress bar
#pragma once
#include <QDockWidget>
#include <QScrollArea>
#include <QWidget>

namespace NoobWarrior {
class BackgroundTaskPopupWidget : public QDockWidget {
    Q_OBJECT
    friend class BackgroundTasks;
public:
    BackgroundTaskPopupWidget(QWidget *parent = nullptr);
    void InitWidgets();
private:
    QScrollArea* mScrollArea;
    QWidget* mWidget;
};
}