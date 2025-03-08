#define NOOBWARRIOR_NOSHELL_HINT 1
#include <NoobWarrior/Shell.h>

#include <cstring>

int main(int argc, char* argv[]) {
    int ret = 0;
    bool startShell = true;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--no-shell", strlen(argv[i])) == 0)
            startShell = false;
    }
    if (startShell) {
        puts("\x1b[34mDon't want the CLI to open a shell? Start the program with the --no-shell flag.\x1b[0m");
        ret = NoobWarrior::Shell::Open(stdout);
    }
    return ret;
}