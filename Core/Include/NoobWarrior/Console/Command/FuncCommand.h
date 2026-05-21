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
// File: FuncCommand.h
// Started by: Hattozo
// Started on: 5/20/2026
// Description:
#pragma once
#include <NoobWarrior/Console/Command/Command.h>

#include <functional>

namespace NoobWarrior {
class FuncCommand : public Command {
public:
    FuncCommand(std::function<void(CommandContext& ctx)>);
    ~FuncCommand() override = default;
    void Main(CommandContext& ctx) override;
private:
    std::function<void(CommandContext& ctx)> mFunc;
};
}