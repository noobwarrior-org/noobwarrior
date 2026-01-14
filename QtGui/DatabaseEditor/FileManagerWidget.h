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
// File: FileManagerWidget.h
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#pragma once
#include <QDockWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>

namespace NoobWarrior {
class FileManagerWidget : public QDockWidget {
    Q_OBJECT
public:
    FileManagerWidget(QWidget *parent = nullptr);
    void Refresh(const QString &address = "/");
private:
    void InitWidgets();
    
    QWidget* MainWidget;
    QVBoxLayout* MainLayout;

    QLineEdit* AddressBar;
};
}
