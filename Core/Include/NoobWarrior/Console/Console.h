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
// File: Console.h
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#pragma once
#include <NoobWarrior/Console/Command/Command.h>

#include <string>
#include <unordered_map>
#include <sstream>
#include <ostream>
#include <istream>
#include <memory>

namespace NoobWarrior {
class Core;
class Console {
public:
    Console(Core* core, std::ostream* out = nullptr, std::istream* in = nullptr);
    ~Console();

    int Exec();

    void Stop();
    void RegisterCommand(const std::string &name, std::unique_ptr<Command> command);

    std::ostream* GetOutputStream();
    std::istream* GetInputStream();
protected:
    std::unordered_map<std::string, std::unique_ptr<Command>> mCommands;
private:
    bool mRunning;
    Core* mCore;
    std::ostream* mOut;
    std::istream* mIn;
    bool mOwningInStream;
    bool mOwningOutStream;
};
}