/************************************************
 *
 * @File: island.c
 * @Purpose: Island process (a port). Usage: ./island <config.dat> <stock.db>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/09/2026
 *
 ************************************************/
#define _GNU_SOURCE

#include <signal.h>

#include "islandConfig.h"

volatile int gnStop = 0;

/************************************************
 *
 * @Name: closePort
 * @Def: SIGINT handler: only raises the stop flag, main does the cleanup.
 * @Arg: In: nSignum = signal received
 * @Ret: -
 *
 ************************************************/
void closePort(int nSignum) {

    (void) nSignum;
    gnStop = 1;
}

int main(int argc, char *argv[]) {

    IslandConfig stConfig;
    char *psMessage = NULL;
    char *psShips = "ships";

    if (3 != argc) {
        printError("Usage: ./island <config.dat> <stock.db>\n");
        return 1;
    }
    if (-1 == loadIslandConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }

    if (1 == stConfig.nCapacity) {
        psShips = "ship";
    }
    if (-1 != asprintf(&psMessage, "Island %s initialized.\nPort capacity: %d %s.\n"
            "%d sea routes loaded.\n%d products available.\n",
            stConfig.psName, stConfig.nCapacity, psShips, stConfig.nNumRoutes, stConfig.nNumProducts)) {
        printString(psMessage);
        free(psMessage);
    }

    signal(SIGINT, closePort);
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