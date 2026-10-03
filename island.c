/************************************************
 *
 * @File: island.c
 * @Purpose: Island process (a port). Usage: ./island <config.dat> <stock.db>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 03/10/2026
 *
 ************************************************/

#include "islandConfig.h"
#include <signal.h>

int gnStop = 0;

/************************************************
 *
 * @Name: handleInterrupt
 * @Def: SIGINT (CTRL+C) handler: it only raises the stop flag.
 * @Arg: In: nSignal = signal received (unused)
 * @Ret: -
 *
 ************************************************/
void handleInterrupt(int nSignal) {

    (void) nSignal;
    gnStop = 1;
}

int main(int argc, char *argv[]) {

    IslandConfig stConfig;
    char *psMessage = NULL;

    if (3 != argc) {
        printError("Usage: ./island <config.dat> <stock.db>\n");
        return 1;
    }
    if (-1 == loadIslandConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt)) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeIslandConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Island %s initialized.\nPort capacity: %d %s.\n%d sea routes loaded.\n%d products available.\n",
                    stConfig.psName, stConfig.nCapacity, (1 == stConfig.nCapacity) ? "ship" : "ships",
                    stConfig.nNumRoutes, stConfig.nNumProducts)) {
        printString(psMessage);
        free(psMessage);
    }

    while (0 == gnStop) {
        pause();
    }

    if (-1 != asprintf(&psMessage, "\n%s closes its port.\n", stConfig.psName)) {
        printString(psMessage);
        free(psMessage);
    }
    freeIslandConfig(&stConfig);
    return 0;
}