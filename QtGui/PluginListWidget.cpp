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
// File: PluginListWidget.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "PluginListWidget.h"

#include "Application.h"
#include <NoobWarrior/NoobWarrior.h>
#include <QDir>
#include <QFileInfo>

using namespace NoobWarrior;

PluginListWidget::PluginListWidget(QWidget* parent) : QListWidget(parent) {
    Refresh();
}

PluginListWidget::~PluginListWidget() {
    
}

void PluginListWidget::Refresh() {
    clear();

    std::filesystem::path dbPath = gApp->GetCore()->GetUserDataDir() / "plugins";
    QDir directory(QString::fromStdString(dbPath.string()));
    
    QStringList filters;
    filters << "*.zip";
    directory.setNameFilters(filters);

    QFileInfoList fileList = directory.entryInfoList(QDir::Filter::Files | QDir::Filter::Dirs);
    for (const QFileInfo& fileInfo : fileList) {
    }
}
