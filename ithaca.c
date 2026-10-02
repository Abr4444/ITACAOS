/************************************************
 *
 * @File: ithaca.c
 * @Purpose: Ithaca process (central server).
 *           Usage: ./ithaca <config.dat> <voyages.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/
#define _GNU_SOURCE

#include "ithacaConfig.h"
#include "interrupt.h"

int main(int argc, char *argv[]) {

    IthacaConfig stConfig;
    char *psMessage = NULL;

    if (3 != argc) {
        printError("Usage: ./ithaca <config.dat> <voyages.dat>\n");
        return 1;
    }
    if (-1 == loadIthacaConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }
    if (-1 == setUpInterrupt()) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeIthacaConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n",
            stConfig.numVoyages)) {
        printString(psMessage);
        free(psMessage);
    }

    // pause() sleeps without using CPU until a signal arrives
    while (0 == gnStop) {
        pause();
    }

    printString("\nIthaca closes the harbor.\n");
    freeIthacaConfig(&stConfig);
    return 0;
}