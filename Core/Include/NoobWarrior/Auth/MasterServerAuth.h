// === noobWarrior ===
// File: MasterServerAuth.h
// Started by: Hattozo
// Started on: 11/7/2025
// Description:
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/NoobWarriorAuth.h>

namespace NoobWarrior {
class MasterServerAuth : public NoobWarriorAuth {
public:
    MasterServerAuth(Config *config);
    std::string GetName() override;
};
}