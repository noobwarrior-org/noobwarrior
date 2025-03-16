// === noobWarrior ===
// File: RccServiceInvoker.cpp
// Started by: Hattozo
// Started on: 2/27/2025
// Description: Starts RccService child process and handles sending SOAP requests to it.
#include <NoobWarrior/RccServiceInvoker.h>

using namespace NoobWarrior;

NoobWarrior::RccService::RccServiceInvoker *NoobWarrior::RccService::gRccService = nullptr;