/************************************************
 *
 * @File: odysseus.c
 * @Purpose: Odysseus process (the ship). Usage: ./odysseus <config.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 03/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "ship.h"
#include "sailing.h"
#include <signal.h>

#define PROMPT "$ "

int gnStop = 0;

/************************************************
 *
 * @Name: handleInterrupt
 * @Def: SIGINT (CTRL+C) handler: raises the stop flag and closes stdin.
 *       signal() makes the kernel restart a read() interrupted by CTRL+C,
 *       so closing stdin is what makes that read() fail and the
 *       terminal loop end.
 * @Arg: In: nSignal = signal received (unused)
 * @Ret: -
 *
 ************************************************/
void handleInterrupt(int nSignal) {

    (void) nSignal;
    gnStop = 1;
    close(STDIN_FILENO);
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
static void runTerminal(Ship *pstShip) {

    Command stCommand;
    char *psLine = NULL;

    printString(PROMPT);
    psLine = readLine(STDIN_FILENO);
    while (0 == gnStop && NULL != psLine) {
        switch (parseCommand(psLine, &stCommand)) {
            case PARSE_OK:
                executeCommand(pstShip, &stCommand);
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
        psLine = NULL;
        //ships lost at sea end the program without asking for more commands
        if (SHIP_LOST != pstShip->nState) {
            printString(PROMPT);
            psLine = readLine(STDIN_FILENO);
        }
    }
    free(psLine);
    printString("\n");
}

int main(int argc, char *argv[]) {

    OdysseusConfig stConfig;
    Ship stShip;
    char *psMessage = NULL;

    if (2 != argc) {
        printError("Usage: ./odysseus <config.dat>\n");
        return 1;
    }
    if (-1 == loadOdysseusConfig(argv[1], &stConfig)) {
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt) || SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeOdysseusConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Odysseus %s is ready to sail.\n", stConfig.psName)) {
        printString(psMessage);
        free(psMessage);
    }
    initShip(&stShip, &stConfig);
    runTerminal(&stShip);

    leavePort(&stShip);
    freeShip(&stShip);

    freeOdysseusConfig(&stConfig);
    return 0;
}