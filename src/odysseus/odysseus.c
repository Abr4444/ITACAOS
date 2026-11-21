/************************************************
 * @File: odysseus.c
 * @Purpose: Odysseus process (the ship). Usage: ./odysseus <config.dat>
 * @Date: 03/10/2026
 ************************************************/

#define _GNU_SOURCE
#include "ship.h"
#include "sailing.h"
#include <signal.h>
#include "state.h"

int gnStop = 0;

/************************************************
 * @Name: handleInterrupt
 * @Def: SIGINT (CTRL+C) handler: raises the stop flag and closes stdin.
 *       main installs it without SA_RESTART, so a read() blocked on a
 *       socket fails at once. Closing stdin also ends a terminal read()
 *       that starts right after CTRL+C.
 * @Arg: In: nSignal = signal received (unused)
 * @Ret: -
 ************************************************/
void handleInterrupt(int nSignal) {

    (void) nSignal;
    gnStop = 1;
    close(STDIN_FILENO);
}

/************************************************
 * @Name: runTerminal
 * @Def: Reads and parses commands until CTRL+C or end of input.
 *       Phase 1 only checks the syntax, it does not run the command yet.
 * @Arg: -
 * @Ret: -
 ************************************************/
static void runTerminal(Ship *pstShip) {
    char *psLine = NULL;

    printString(PROMPT);
    psLine = readLine(STDIN_FILENO);
    while (0 == gnStop && NULL != psLine) {
        runLine(pstShip, psLine);
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
    struct sigaction stAction;

    if (2 != argc) {
        printError("Usage: ./odysseus <config.dat>\n");
        return 1;
    }
    if (-1 == loadOdysseusConfig(argv[1], &stConfig)) {
        return 1;
    }
    initShip(&stShip, &stConfig);
    stShip.pnStop = &gnStop;
    if (-1 == loadState(&stShip)) {
        printError("Error: could not recover the saved state.\n");
        freeShip(&stShip);
        freeOdysseusConfig(&stConfig);
        return 1;
    }
    memset(&stAction, 0, sizeof(stAction));
    stAction.sa_handler = handleInterrupt;
    sigemptyset(&stAction.sa_mask);
    stAction.sa_flags = 0;
    if (-1 == sigaction(SIGINT, &stAction, NULL) || SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        freeShip(&stShip);
        printError("Error: could not install the CTRL+C handler.\n");
        freeOdysseusConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Odysseus %s is ready to sail.\n", stConfig.psName)) {
        printString(psMessage);
        free(psMessage);
    }
    runTerminal(&stShip);

    leavePort(&stShip);
    freeShip(&stShip);

    freeOdysseusConfig(&stConfig);
    return 0;
}