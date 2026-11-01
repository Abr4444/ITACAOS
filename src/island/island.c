/************************************************
 *
 * @File: island.c
 * @Purpose: Island process (a port). Usage: ./island <config.dat> <stock.db>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 03/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "islandConfig.h"
#include "islandServer.h"

#include "socket.h"
#include <signal.h>

//listening socket, shut down by CTRL+C
int gnServerFd = -1;

/* @Name: handleInterrupt
 * @Def: CTRL+C: shuts the listening socket down to stop the server.
 * @Arg: In: nSignal = signal received (unused)
 * @Ret: - */
void handleInterrupt(int nSignal) {

    (void) nSignal;
    shutdown(gnServerFd, SHUT_RDWR);
}

int main(int argc, char *argv[]) {

    IslandConfig stConfig;
    IslandServer stIsland;
    char *psMessage = NULL;
    char *psShips = "ships";

    if (3 != argc) {
        printError("Usage: ./island <config.dat> <stock.db>\n");
        return 1;
    }
    if (-1 == loadIslandConfig(argv[1], argv[2], &stConfig)) {
        return 1;
    }

    gnServerFd = openListenConn(stConfig.psIp, stConfig.nPort);
    if (-1 == gnServerFd) {
        printError("Error: could not open the island's port. Check the IP and port of the config file.\n");
        freeIslandConfig(&stConfig);
        return 1;
    }
    if (SIG_ERR == signal(SIGINT, handleInterrupt) || SIG_ERR == signal(SIGPIPE, SIG_IGN)) {
        printError("Error: could not install the signal handlers.\n");
        close(gnServerFd);
        freeIslandConfig(&stConfig);
        return 1;
    }
    if (-1 == initIslandServer(&stIsland, &stConfig)) {
        close(gnServerFd);
        freeIslandConfig(&stConfig);
        return 1;
    }

    if (1 == stConfig.nCapacity) {
        psShips = "ship";
    }
    if (-1 != asprintf(&psMessage, "\nIsland %s initialized.\nPort capacity: %d %s.\n%d sea routes loaded.\n%d products available.\n",
                    stConfig.psName, stConfig.nCapacity, psShips,
                    stConfig.nNumRoutes, stConfig.nNumProducts)) {
        printString(psMessage);
        free(psMessage);
    }

    runIslandServer(&stIsland, gnServerFd);

    closeIslandServer(&stIsland);

    close(gnServerFd);
    if (-1 != asprintf(&psMessage, "\n%s closes its port.\n", stConfig.psName)) {
        printString(psMessage);
        free(psMessage);
    }
    freeIslandConfig(&stConfig);
    return 0;
}
