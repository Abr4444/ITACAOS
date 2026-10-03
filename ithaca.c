/************************************************
 *
 * @File: ithaca.c
 * @Purpose: Ithaca process (central server).
 *           Usage: ./ithaca <config.dat> <voyages.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/

#include "ithacaConfig.h"
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

    IthacaConfig stConfig;

    if (3 != argc) {
        printError("Usage: ./ithaca <config.dat> <voyages.dat>\n");
        return 1;
    }
    if (-1 == loadIthacaConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt)) {
        printError("Error: could not install the CTRL+C handler.\n");
        freeIthacaConfig(&stConfig);
        return 1;
    }

    asprintf("Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", stConfig.numVoyages);
    while (0 == gnStop) {
        pause();
    }

    printString("\nIthaca closes the harbor.\n");
    freeIthacaConfig(&stConfig);
    return 0;
}