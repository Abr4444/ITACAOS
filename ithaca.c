/************************************************
 *
 * @File: ithaca.c
 * @Purpose: Ithaca process (central server).
 *           Usage: ./ithaca <config.dat> <voyages.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/
#define _GNU_SOURCE
#include <signal.h>

#include "ithacaConfig.h"


int stop = 0;

/************************************************
 *
 * @Name: closeHarbor
 * @Def: SIGINT handler: only raises the stop flag, main does the cleanup.
 * @Arg: In: signum = signal received
 * @Ret: -
 *
 ************************************************/
void closeHarbor(int signum) {

    (void) signum;
    stop = 1;
}

int main(int argc, char *argv[]) {

    IthacaConfig config;
    char *message = NULL;

    if (3 != argc) {
        printError("Usage: ./ithaca <config.dat> <voyages.dat>\n");
        return 1;
    }
    if (-1 == loadIthacaConfig(argv[1], argv[2], &config)) {
        return 1;
    }

    if (-1 != asprintf(&message, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n",
            config.numVoyages)) {
        printString(message);
        free(message);
    }

    // pause() sleeps without using CPU until a signal arrives
    signal(SIGINT, closeHarbor);
    while (0 == stop) {
        pause();
    }

    printString("\nIthaca closes the harbor.\n");
    freeIthacaConfig(&config);
    return 0;
}