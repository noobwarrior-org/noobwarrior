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
// File: OrganizerWidget.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description: A widget that allows the user to reorganize content in a way they like by putting them into directories
// This is different from the regular content browser because that organizes stuff on its own according to what it's
// seeing in the SQLite database. This lets you organize it yourself
// This is just like a traditional file manager, with tree views and rearranging files and all of that shit.
#include "FileManagerWidget.h"
#include "Sdk.h"

using namespace NoobWarrior;

FileManagerWidget::FileManagerWidget(QWidget *parent) : QDockWidget(parent)
{
    assert(dynamic_cast<Sdk*>(this->parent()) != nullptr);
    setWindowTitle("File Manager");
    InitWidgets();
    Refresh();
}

void FileManagerWidget::InitWidgets() {
    MainWidget = new QWidget(this);
    setWidget(MainWidget);

    MainLayout = new QVBoxLayout(MainWidget);

    AddressBar = new QLineEdit();
    AddressBar->setPlaceholderText("Address");
    MainLayout->addWidget(AddressBar);

    connect(AddressBar, &QLineEdit::returnPressed, [this]() {
        Refresh(AddressBar->text());
    });
}

void FileManagerWidget::Refresh(const QString &address) {
    AddressBar->setText(address);
}