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
// File: ServerInformationSidebar.h
// Started by: Hattozo
// Started on: 4/24/2026
// Description:
#pragma once
#include <QDockWidget>

namespace NoobWarrior {
class ServerInformationSidebar : public QDockWidget {
    Q_OBJECT
public:
    ServerInformationSidebar(QWidget* parent = nullptr);
    ~ServerInformationSidebar();
protected:
    void InitWidgets();
};
}
