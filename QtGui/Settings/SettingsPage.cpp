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
// File: SettingsPage.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#include "SettingsPage.h"

#include <QLabel>

using namespace NoobWarrior;

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent),
    Layout(new QVBoxLayout(this))
{}

void SettingsPage::Init() {
    auto nameAndDescLayout = new QVBoxLayout();

    auto title = new QLabel(GetTitle());
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font = title->font();
    font.setBold(true);
    font.setPointSize(14);
    title->setFont(font);

    auto desc = new QLabel(GetDescription());
    desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    desc->setWordWrap(true);

    nameAndDescLayout->addWidget(title);
    nameAndDescLayout->addWidget(desc);
    Layout->addLayout(nameAndDescLayout);
}
