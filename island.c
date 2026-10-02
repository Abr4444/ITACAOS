/************************************************
 *
 * @File: island.c
 * @Purpose: Island process (a port). Usage: ./island <config.dat> <stock.db>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/
#define _GNU_SOURCE

#include "islandConfig.h"
#include "interrupt.h"

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
    if (-1 == setUpInterrupt()) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeIslandConfig(&stConfig);
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

    // pause() sleeps without using CPU until a signal arrives
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