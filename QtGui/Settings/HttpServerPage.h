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
// File: HttpServerPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#pragma once
#include "SettingsPage.h"

namespace NoobWarrior {
class HttpServerPage : public SettingsPage {
public:
    HttpServerPage(QWidget *parent = nullptr);
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
};
}
