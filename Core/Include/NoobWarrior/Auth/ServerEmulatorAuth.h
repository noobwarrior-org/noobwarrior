// === noobWarrior ===
// File: ServerEmulatorAuth.h
// Started by: Hattozo
// Started on: 11/10/2025
// Description:
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/NoobWarriorAuth.h>

namespace NoobWarrior {
class ServerEmulatorAuth : public NoobWarriorAuth {
public:
    ServerEmulatorAuth(Config *config);
    std::string GetName() override;
};
}