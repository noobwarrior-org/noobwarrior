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
// File: EmuDbProject.h
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#pragma once
#include "Project.h"

#include <NoobWarrior/EmuDb/EmuDb.h>

namespace NoobWarrior {
class EmuDbProject : public Project {
public:
    EmuDbProject(const std::string &path = ":memory:");
    ~EmuDbProject();

    std::string GetTitle() override;
    QIcon GetIcon() override;

    void OnShown(Sdk*) override;
    void OnHidden(Sdk*) override;
protected:
    EmuDb *mDb;
};
}