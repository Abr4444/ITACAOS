/************************************************
 * @File: sailing.c
 * @Purpose: SAIL: leaving a port, sailing a known route, docking and waiting offshore.
 * @Date: 31/10/2026
 ************************************************/

#define _GNU_SOURCE
#include "sailing.h"

/************************************************
 * @Name: findRoute
 * @Def: SAIL is only valid along a direct route of the current location:
 *       from Ithaca only to Aeaea, from an island only to the islands
 *       its map connects it with.
 * @Arg: In: pstShip = the ship
 *       In: psName = island the user wants to sail to
 * @Ret: The destination, or NULL if there is no known direct route.
 ************************************************/
static KnownIsland *findRoute(Ship *pstShip, char *psName) {
    KnownIsland *pstFrom = NULL;

    if (0 == strcmp(pstShip->psLocation, ITHACA_NAME)) {
        if (0 == strcasecmp(psName, pstShip->pstConfig->psAeaeaName))
            return findIsland(&pstShip->stMap, psName);
        return NULL;
    }
    pstFrom = findIsland(&pstShip->stMap, pstShip->psLocation);
    for (int i = 0; NULL != pstFrom && i < pstFrom->nNumRoutes; i++) {
        if (0 == strcasecmp(pstFrom->ppsRoutes[i], psName))
            return findIsland(&pstShip->stMap, psName);
    }
    return NULL;
}
/************************************************
 * @Name: eatRation
 * @Def: The crew eats RATION_KG kg, taken from the cargo in order: when a
 *       food runs out, the rest comes from the next one. If there is less
 *       than a full ration, they eat what is left.
 * @Arg: In/Out: pstConfig = the configuration that holds the cargo
 * @Ret: The provisions left, in kg.
 ************************************************/
static int eatRation(OdysseusConfig *pstConfig) {

    int nToEat = RATION_KG;
    int nEaten = 0;
    int nLeft = 0;

    for (int i = 0; i < pstConfig->nNumCargo; i++) {
        nEaten = pstConfig->pstCargo[i].nKg;
        if (nEaten > nToEat)
            nEaten = nToEat;
        pstConfig->pstCargo[i].nKg -= nEaten;
        nToEat -= nEaten;
        nLeft += pstConfig->pstCargo[i].nKg;
    }
    return nLeft;
}
/************************************************
 * @Name: waitForBerth
 * @Def: The port is full: waits offshore until the island sends DOCKED.
 *       select() sleeps until the island writes or RATION_SECONDS pass,
 *       so the wait uses no CPU. Every timeout costs a ration, and with
 *       no provisions left the ship is lost at sea.
 * @Arg: In/Out: pstShip = the ship, connected to the island
 *       In: pstIsland = the island it waits for
 * @Ret: 1 if the ship docked, 0 otherwise (the connection is closed).
 ************************************************/
static int waitForBerth(Ship *pstShip, KnownIsland *pstIsland) {

    fd_set stReadSet;
    struct timeval stTimeout;
    Frame stFrame;
    char *psMessage = NULL;
    int nReady = 0;
    int nLeft = 0;
    int nDocked = -1;

    while (-1 == nDocked) {
        // select() changes both, so they are set again before every call
        FD_ZERO(&stReadSet);
        FD_SET(pstShip->nFd, &stReadSet);
        stTimeout.tv_sec = RATION_SECONDS;
        stTimeout.tv_usec = 0;

        nReady = select(pstShip->nFd + 1, &stReadSet, NULL, NULL, &stTimeout);
        if (-1 == nReady) {
            // CTRL+C interrupts select(): the program is closing
            nDocked = 0;
        } else if (0 == nReady) {
            nLeft = eatRation(pstShip->pstConfig);
            if (0 == nLeft) {
                if (-1 != asprintf(&psMessage, "\nThe crew has run out of provisions.\nOdysseus %s has been lost at sea.\n",
                    pstShip->pstConfig->psName)) {
                    printString(psMessage);
                    free(psMessage);
                }
                pstShip->nState = SHIP_LOST;
                nDocked = 0;
            } else if (-1 != asprintf(&psMessage, "\n>>> The crew consumes another ration.\nProvisions remaining: %d kg\n", nLeft)) {
                printString(psMessage);
                free(psMessage);
            }
        } else if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            if (-1 != asprintf(&psMessage, "Error: lost the connection with %s.\n", pstIsland->psName)) {
                printString(psMessage);
                free(psMessage);
            }
            nDocked = 0;
        } else if (TYPE_ARRIVAL == stFrame.nType && FLAGS_OK == stFrame.nFlags && 0 == strcmp(stFrame.sData, "DOCKED")) {
            if (-1 != asprintf(&psMessage, "\n>>> %s has granted access to the port.\n", pstIsland->psName)) {
                printString(psMessage);
                free(psMessage);
            }
            nDocked = 1;
        }
    }

    if (0 == nDocked)
        closeConnection(pstShip);
    return nDocked;
}
/************************************************
 * @Name: arriveAt
 * @Def: Connects to an island and asks to dock (ARRIVAL). 
 *       The island answers DOCKED, or WAIT if its port is full and DOCKED later.
 * @Arg: In/Out: pstShip = the ship
 *       In: pstIsland = the island it sails to
 * @Ret: -
 ************************************************/
static void arriveAt(Ship *pstShip, KnownIsland *pstIsland) {

    Frame stFrame;
    char *psMessage = NULL;

    pstShip->nFd = openClientConn(pstIsland->psIp, pstIsland->nPort);
    if (-1 == pstShip->nFd) {
        if (-1 != asprintf(&psMessage, "Error: could not reach %s.\n", pstIsland->psName)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (-1 == sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_REQUEST, pstShip->pstConfig->psName) ||
        -1 == receiveFrame(pstShip->nFd, &stFrame) || TYPE_ARRIVAL != stFrame.nType) {
        if (-1 != asprintf(&psMessage, "Error: %s did not answer.\n", pstIsland->psName)) {
            printString(psMessage);
            free(psMessage);
        }
        closeConnection(pstShip);
        return;
    }
    if (FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "%s refused us: %s\n", pstIsland->psName, stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        closeConnection(pstShip);
        return;
    }

    if (-1 != asprintf(&psMessage, "\nConnected to %s.\n", pstIsland->psName)) {
        printString(psMessage);
        free(psMessage);
    }
    if (0 == strcmp(stFrame.sData, "WAIT")) {
        printString("The harbor is full. We drop anchor and wait.\n");
        if (0 == waitForBerth(pstShip, pstIsland))
            return;
    } else if (-1 != asprintf(&psMessage, "%s has granted access to the port.\n", pstIsland->psName)) {
        printString(psMessage);
        free(psMessage);
    }

    pstShip->nState = SHIP_DOCKED;
    pstShip->psLocation = pstIsland->psName;
    if (0 != pstShip->nVoyageId && 0 == strcasecmp(pstShip->psDestination, pstIsland->psName)) {
        if (-1 != asprintf(&psMessage, "We have reached %s, the destination of our voyage.\n", pstIsland->psName)) {
            printString(psMessage);
            free(psMessage);
        }
    }
}
/************************************************
 * @Name: sail
 * @Def: SAIL <island>: leaves the current place and sails to an island
 *       with a known direct route. It works with or without a voyage.
 * @Arg: In/Out: pstShip = the ship
 *       In: psName = island to sail to
 * @Ret: -
 ************************************************/
void sail(Ship *pstShip, char *psName) {
    KnownIsland *pstIsland = findRoute(pstShip, psName);
    char *psMessage = NULL;

    if(NULL == pstIsland){
        if (-1 != asprintf(&psMessage, "We can't sail to %s from here...\n", psName)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (-1 != asprintf(&psMessage, "Leaving %s...\nSailing to %s...\n", pstShip->psLocation, pstIsland->psName)) {
        printString(psMessage);
        free(psMessage);
    }
    leavePort(pstShip);
    arriveAt(pstShip, pstIsland);
}

/************************************************
 * @Name: leavePort
 * @Def: Says goodbye to the place the ship is connected to (0x05 to
 *       Ithaca, 0x16 to an island) and closes the connection. If the
 *       other side has crashed, the goodbye fails and closing is enough.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 ************************************************/
void leavePort(Ship *pstShip) {
    Frame stFrame;
    int nType = TYPE_DEPARTURE;

    if (SHIP_CONNECTED_ITHACA == pstShip->nState || SHIP_DOCKED == pstShip->nState) {
        if (SHIP_CONNECTED_ITHACA == pstShip->nState)
            nType = TYPE_DISCONNECT_ITHACA;
        if (0 == sendFrame(pstShip->nFd, nType, FLAGS_REQUEST, NULL))
            receiveFrame(pstShip->nFd, &stFrame);
    }
    closeConnection(pstShip);
    if (0 == strcmp(pstShip->psLocation, ITHACA_NAME))
        pstShip->nState = SHIP_AT_ITHACA;
    else
        pstShip->nState = SHIP_AT_SEA;
}