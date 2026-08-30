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
// File: Project.cpp
// Started by: Hattozo
// Started on: 2/2/2026
// Description:
#include "Project.h"
#include "Sdk/Sdk.h"

using namespace NoobWarrior;

Project::Project() : mSdk(nullptr),
    mTabWidget(new QTabWidget(nullptr)) // dw the sdk window will reparent it for us
{
    mTabWidget->setProperty("Project", QVariant::fromValue(this));
    mTabWidget->setMovable(true);
    mTabWidget->setTabsClosable(true);
    QObject::connect(mTabWidget, &QTabWidget::tabCloseRequested, mTabWidget,
                     [tabs = mTabWidget](int index) {
        if (QWidget* page = tabs->widget(index))
            page->close();
    });
}

Project::~Project() {
    OnHidden();
    mTabWidget->deleteLater();
}

QString Project::GetOpenFailMsg() {
    return GetFailMsg();
}

QString Project::GetSaveFailMsg() {
    return "N/A";
}

std::filesystem::path Project::GetFilePath() {
    return {};
}

void Project::OnShown() { }
void Project::OnHidden() { }

void Project::TryClose() {

}

QTabWidget* Project::GetTabWidget() {
    return mTabWidget;
}
