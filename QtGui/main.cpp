// === noobWarrior ===
// File: main.cpp
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Main entrypoint for Qt application
#include "Application.h"

int main(int argc, char **argv) {
    Q_INIT_RESOURCE(resources);
    NoobWarrior::Application app(argc, argv);
    return app.Run();
}