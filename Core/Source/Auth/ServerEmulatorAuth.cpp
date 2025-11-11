// === noobWarrior ===
// File: ServerEmulatorAuth.cpp
// Started by: Hattozo
// Started on: 11/10/2025
// Description:
#include <NoobWarrior/Auth/ServerEmulatorAuth.h>

using namespace NoobWarrior;

ServerEmulatorAuth::ServerEmulatorAuth(Config *config) : NoobWarriorAuth(config) {}

std::string ServerEmulatorAuth::GetName() {
    return "server_emulator";
}
