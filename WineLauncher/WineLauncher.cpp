// === noobWarrior ===
// File: WineLauncher.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Sole purpose of this program is to serve as a simple DLL injector running under a wine environment.
// Usually on Windows systems this process is handled by the noobWarrior.exe program since we can just call the Windows API directly
// But we are not on Windows and we are running noobWarrior under an entirely different OS so we must run this separately in a Wine environment.
#include <NoobWarrior/Injector.h>

#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int showCmd) {
    NoobWarrior::Inject(HUGE_VAL, "./noobwarrior-hook.dll");
    return 0;
}