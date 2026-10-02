/************************************************
 *
 * @File: odysseus.c
 * @Purpose: Odysseus process (the ship). Usage: ./odysseus <config.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/
#define _GNU_SOURCE

#include "odysseusConfig.h"
#include "terminal.h"
#include "interrupt.h"

#define PROMPT "$ "

/************************************************
 *
 * @Name: showResult
 * @Def: Tells the user how the command was parsed (Phase 1 only
 *       checks syntax, it does not run the command yet).
 * @Arg: In: nResult = value returned by parseCommand
 *       In: pstCommand = parsed command
 * @Ret: -
 *
 ************************************************/
static void showResult(int nResult, Command *pstCommand) {

    switch (nResult) {
        case PARSE_OK:
            printString("Command OK\n");
            break;

        case PARSE_BAD_SYNTAX:
            printString(pstCommand->psUsage);
            break;

        case PARSE_UNKNOWN:
            printString("Unknown command\n");
            break;

        default:
            // PARSE_EMPTY: a blank line just shows the prompt again
            break;
    }
}

/************************************************
 *
 * @Name: runTerminal
 * @Def: Reads and parses commands until CTRL+C or end of input.
 * @Arg: -
 * @Ret: -
 *
 ************************************************/
static void runTerminal(void) {

    Command stCommand;
    char *psLine = NULL;
    int nResult = 0;

    printString(PROMPT);
    // readInputLine fails on CTRL+C (EINTR), end of input or lack of memory
    while (0 == readInputLine(STDIN_FILENO, &psLine)) {
        nResult = parseCommand(psLine, &stCommand);
        showResult(nResult, &stCommand);
        free(psLine);
        printString(PROMPT);
    }
}

int main(int argc, char *argv[]) {

    OdysseusConfig stConfig;
    char *psMessage = NULL;

    if (2 != argc) {
        printError("Usage: ./odysseus <config.dat>\n");
        return 1;
    }
    if (-1 == loadOdysseusConfig(argv[1], &stConfig)) {
        return 1;
    }
    if (-1 == setUpInterrupt()) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeOdysseusConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Welcome aboard, %s. Type a command.\n", stConfig.name)) {
        printString(psMessage);
        free(psMessage);
    }
    runTerminal();

    printString("\nOdysseus drops anchor. Goodbye.\n");
    freeOdysseusConfig(&stConfig);
    return 0;
}