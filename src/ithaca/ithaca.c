/************************************************
 *
 * @File: ithaca.c
 * @Purpose: Ithaca process (central server).
 *           Usage: ./ithaca <config.dat> <voyages.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 03/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "ithacaConfig.h"
#include "socket.h"
#include <signal.h>

//listening socket, shut down by CTRL+C
int gnServerFd = -1;

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
    shutdown(gnServerFd, SHUT_RDWR);
}

int main(int argc, char *argv[]) {

    IthacaConfig stConfig;
    char *psMessage = NULL;
    int nClientFd = 0;

    if (3 != argc) {
        printError("Usage: ./ithaca <config.dat> <voyages.dat>\n");
        return 1;
    }
    if (-1 == loadIthacaConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }

    gnServerFd = openListenConn(stConfig.psIp, stConfig.nPort);
    if (-1 == gnServerFd) {
        printError("Error: could not open Ithaca's port. Check the IP and port of the config file.\n");
        freeIthacaConfig(&stConfig);
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt) || SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        printError("Error: could not install the signal handlers.\n");
        close(gnServerFd);
        freeIthacaConfig(&stConfig);
        return 1;
    }

    if (-1 != asprintf(&psMessage, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", stConfig.nNumVoyages)) {
        printString(psMessage);
        free(psMessage);
    }

    // Temporary, step 5 replaces it: greet each connection and hang up
    nClientFd = accept(gnServerFd, NULL, NULL);
    while (-1 != nClientFd) {
        printString("Someone connected.\n");
        close(nClientFd);
        nClientFd = accept(gnServerFd, NULL, NULL);
    }

    close(gnServerFd);
    printString("\nIthaca closes the harbor.\n");
    freeIthacaConfig(&stConfig);
    return 0;
}