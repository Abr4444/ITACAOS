/************************************************
 *
 * @File: odysseus.c
 * @Purpose: Odysseus process (the ship). Usage: ./odysseus <config.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/

#include "odysseusConfig.h"
#include "terminal.h"
#include <signal.h>

#define PROMPT "$ "

OdysseusConfig gstConfig;

/************************************************
 *
 * @Name: handleInterrupt
 * @Def: SIGINT (CTRL+C) handler: frees the configuration and exits.
 * @Arg: In: nSignal = signal received (unused)
 * @Ret: -
 *
 ************************************************/
void handleInterrupt(int nSignal) {

    (void) nSignal;
    printString("\n");
    freeOdysseusConfig(&gstConfig);
    exit(0);
}

/************************************************
 *
 * @Name: runTerminal
 * @Def: Reads and parses commands until CTRL+C or end of input.
 *       Phase 1 only checks the syntax, it does not run the command yet.
 * @Arg: -
 * @Ret: -
 *
 ************************************************/
static void runTerminal(void) {

    Command stCommand;
    char *psLine = NULL;

    printString(PROMPT);
    while (0 == readInputLine(STDIN_FILENO, &psLine)) {
        switch (parseCommand(psLine, &stCommand)) {
            case PARSE_OK:
                printString("Command OK\n");
                break;
            case PARSE_BAD_SYNTAX:
                printString(stCommand.psUsage);
                break;
            case PARSE_UNKNOWN:
                printString("Unknown command\n");
                break;
            default:
                break;
        }
        free(psLine);
        printString(PROMPT);
    }
    printString("\n");
}

int main(int argc, char *argv[]) {

    if (2 != argc) {
        printError("Usage: ./odysseus <config.dat>\n");
        return 1;
    }
    if (-1 == loadOdysseusConfig(argv[1], &gstConfig)) {
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt)) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeOdysseusConfig(&gstConfig);
        return 1;
    }

    printFormat("Welcome aboard, %s. Type a command.\n", gstConfig.name);
    runTerminal();

    freeOdysseusConfig(&gstConfig);
    return 0;
}