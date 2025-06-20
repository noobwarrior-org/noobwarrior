// === noobWarrior ===
// File: GameLauncher.h
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#pragma once

#include "Menu2D.h"

namespace NoobWarrior {
class GameLauncher {
public:
    GameLauncher();

    int Run();
private:
    bool Running;
    int Width, Height;
    Menu2D Menu;
};
}
