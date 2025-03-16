// === noobWarrior ===
// File: Shell.cpp
// Started by: Hattozo
// Started on: 3/7/2025
// Description: Basic shell that's designed to run noobWarrior functions.
#include <NoobWarrior/Shell.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/AssetRequest.h>

#include <cassert>
#include <cstring>

#define ERROR_MSG "\x1b[31mError:\x1b[0m "

using namespace NoobWarrior;

static bool sRunning;
FILE* Shell::gOut = stdout;

static void Help(int argc, char* argv[]); // forwards declare

static void About() {
    fputs("\x1b[33mnoobWarrior v" NOOBWARRIOR_VERSION "\x1b[0m\n", Shell::gOut);
    fprintf(Shell::gOut, "\x1b[35m%s\x1b[0m\n", "Authors\n" NOOBWARRIOR_AUTHORS "\nContributors\n" NOOBWARRIOR_CONTRIBUTORS "\nAttributions\n" NOOBWARRIOR_ATTRIBUTIONS_BRIEF);
}

static void Auth() {
    fputs("Enter your Roblox username: ", Shell::gOut);
}

static void DownloadAsset(int argc, char** argv) {
    fputs("Downloading Roblox assets\n", Shell::gOut);
    DownloadAssetArgs args;
    args.FileNameStyle = NoobWarrior::AssetFileNameStyle::Raw;
    args.Flags |= DA_FIND_ASSET_IDS_IN_MODEL;
    args.OutDir = "./downloads";
    args.Id = {1818};
    NoobWarrior::DownloadAssets(args);
}

static void RccService(int argc, char** argv) {
    switch (argc) {
    case 1:
        fputs("RCCService parameters:\nstart - Starts RCCService, port is 8080 if not specified\nstop - Stops RCCService\nrestart - Restarts RCCService\n", Shell::gOut);
        break;
    }
}

static Shell::Command sCommands[] = {
    {(void(*))Help, "help", "Gives a list of all available commands."},
    {(void(*))About, "about", "Shows the version number and the people who have worked on noobWarrior."},
    {(void(*))Shell::Close, "exit", "Stops the shell."},
    {(void(*))::Auth, "auth", "Authenticates a Roblox account with noobWarrior"},
    {(void(*))::RccService, "rccservice", "Control the behavior and operations of RCCService"},
    {(void(*))::DownloadAsset, "download-asset", "Downloads assets by supplying them their ID. If you want to sequentially download a list of assets, use a comma-separated list of IDs inside of quotes. Ex: download-asset --id \"5804349700, 12221333, 3697434836\" --out \"./downloads\""}
};

static const char* sAliases[][2] = {
    {"quit", "exit"},
    {"auth", "login"}
};

static void Help(int argc, char* argv[]) {
    fputs("\x1b[32mCommands:\n", Shell::gOut);
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(sCommands); i++)
        fprintf(Shell::gOut, "\x1b[35m%s - \x1b[36m%s\x1b[0m\n", sCommands[i].Name, sCommands[i].Description);
}

// https://stackoverflow.com/a/33780304
// Strips backslashes from quotes
char* unescapeToken(char* token)
{
    char *in = token;
    char *out = token;

    while (*in)
    {
        assert(in >= out);

        if ((in[0] == '\\') && (in[1] == '"'))
        {
            *out = in[1];
            out++;
            in += 2;
        }
        else
        {
            *out = *in;
            out++;
            in++; 
        }
    }
    *out = 0;
    return token;
}

// Returns the end of the token, without changing it.
char* qtok(char* str, char** next)
{
    char *current = str;
    char *start = str;
    int isQuoted = 0;

    // Eat beginning whitespace.
    while (*current && isspace(*current)) current++;
    start = current;

    if (*current == '"')
    {
        isQuoted = 1;
        // Quoted token
        current++; // Skip the beginning quote.
        start = current;
        for (;;)
        {
            // Go till we find a quote or the end of string.
            while (*current && (*current != '"')) current++;
            if (!*current) 
            {
                // Reached the end of the string.
                goto finalize;
            }
            if (*(current - 1) == '\\')
            {
                // Escaped quote keep going.
                current++;
                continue;
            }
            // Reached the ending quote.
            goto finalize; 
        }
    }
    // Not quoted so run till we see a space.
    while (*current && !isspace(*current)) current++;
finalize:
    if (*current)
    {
        // Close token if not closed already.
        *current = 0;
        current++;
        // Eat trailing whitespace.
        while (*current && isspace(*current)) current++;
    }
    *next = current;

    return isQuoted ? unescapeToken(start) : start;
}

int Shell::Open(FILE* out, FILE* in) {
    // DownloadAsset(0, nullptr);
    if (sRunning) return 1;
    sRunning = true;
    Shell::gOut = out;
    char yeah[1024];
    fputs("\x1b[33mnoobWarrior v" NOOBWARRIOR_VERSION " \x1b[35mShell\n\x1b[36mType \"help\" for commands. Type \"about\" to see credits. Type \"exit\" to quit.\x1b[0m\n", Shell::gOut);
    fputs("\x1b[31mYou currently do not have a Roblox account that is authenticated with noobWarrior. This can cause rate-limiting by the Roblox API when downloading assets, so it is advised that you use \"auth\" to authenticate your account.\x1b[0m\n", Shell::gOut);
    while (sRunning) {
        fputs("> ", out);
        if (fgets(yeah, sizeof(yeah), in) != nullptr) {
            ParseCommand(&yeah[0]);
        } else break; // if we're not reading from the input stream anymore
    }
    return 0;
}

void Shell::Close() {
    sRunning = false;
}

void Shell::ParseCommand(char* cmd) {
    cmd[strcspn(cmd, "\n")] = 0; // replace newline with null terminator so that it's more akin to a normal string
    if (strncmp(cmd, "\0", 1) == 0) {
        fprintf(Shell::gOut, ERROR_MSG "Empty command.\n");
        return;
    }
    int argc;
    char** argv;
    for (argc = 0; *cmd != '\0'; argc++) {
        argv = (char**)realloc(argc == 0 ? NULL : argv, argc * sizeof(char*));
        argv[argc] = qtok(cmd, &cmd);
    }
    for (int i = 0; i < NOOBWARRIOR_ARRAY_SIZE(sCommands); i++) {
        if (strncmp(sCommands[i].Name, argv[0], strlen(sCommands[i].Name)) == 0) {
            ((void(*)(int, char**))sCommands[i].Main)(argc, argv);
            return;
        }
    }
    fprintf(Shell::gOut, ERROR_MSG "Command \"%s\" not found.\n", argv[0]);
    free(argv);
}