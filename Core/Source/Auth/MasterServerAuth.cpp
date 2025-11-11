// === noobWarrior ===
// File: MasterServerAuth.cpp
// Started by: Hattozo
// Started on: 11/7/2025
// Description:
#include <NoobWarrior/Auth/MasterServerAuth.h>

using namespace NoobWarrior;

MasterServerAuth::MasterServerAuth(Config *config) : NoobWarriorAuth(config) {}

std::string MasterServerAuth::GetName() {
    return "master_server";
}
