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
// File: EmuDbComboBox.h
// Started by: Hattozo
// Started on: 5/27/2026
// Description:
#pragma once
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <QComboBox>

namespace NoobWarrior {
class EmuDbComboBox : public QComboBox {
    Q_OBJECT
public:
    enum class Mode {
        ShowEntriesInDir, // Shows every database file in the databases folder, even the ones that aren't mounted
        ShowMounted // Shows only the currently mounted databases in the database manager
    };

    EmuDbComboBox(Mode mode = Mode::ShowEntriesInDir, QWidget* parent = nullptr);
    ~EmuDbComboBox();

    void Refresh();
    bool SetDatabase(EmuDb* db);
    EmuDb* GetSelectedDatabase();
private:
    Mode mMode;
};
}