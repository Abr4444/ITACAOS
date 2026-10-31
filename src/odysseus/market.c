#define _GNU_SOURCE
#include "market.h"

/************************************************
 *
 * @Name: lostIsland
 * @Def: The island stopped answering (it crashed or closed): the ship
 *       forgets the connection and stays at sea next to it.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 *
 ************************************************/
static void lostIsland(Ship *pstShip) {

    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, "Error: lost the connection with %s.\n", pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
    closeConnection(pstShip);
    pstShip->nState = SHIP_AT_SEA;
}


/************************************************
 *
 * @Name: buyMap
 * @Def: BUY MAP 1: buys the map of the island where the ship is docked.
 *       The island sends one frame per route ("OK&50&Index&Total&...")
 *       or a single "OK&50&0" if it has no routes.
 * @Arg: In/Out: pstShip = the ship, pays and learns the routes
 * @Ret: -
 *
 ************************************************/
void buyMap(Ship *pstShip) {
    Frame stFrame;
    char *apsFields[BUY_MAP_FIELDS];
    char *psMessage = NULL;
    int nReceived = 0;
    int nTotal = 1;// until the first frame says how many there are
    int nFields = 0;
    int nError = 0;

    if (SHIP_DOCKED != pstShip->nState) {
        printString("You must be docked at an island first.\n");
        return;
    }
    if (MAP_PRICE > pstShip->nGold) {
        printString("You don't have enough gold for the map.\n");
        return;
    }
    if (-1 == sendFrame(pstShip->nFd, TYPE_BUY_MAP, FLAGS_REQUEST, "1")) {
        lostIsland(pstShip);
        return;
    }

    while (nReceived < nTotal && 0 == nError) {
        if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            lostIsland(pstShip);
            return;
        }
        nReceived++;
        if (TYPE_BUY_MAP != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
            // The island sends nothing more after an error
            if (-1 != asprintf(&psMessage, "%s did not sell the map: %s\n", pstShip->psLocation, stFrame.sData)) {
                printString(psMessage);
                free(psMessage);
            }
            nError = 1;
        } else {
            nFields = splitFields(stFrame.sData, apsFields, BUY_MAP_FIELDS);
            if (3 == nFields && 0 == strcmp(apsFields[2], "0"))
                nTotal = 0;
            else if (BUY_MAP_FIELDS == nFields) {
                nTotal = atoi(apsFields[3]);
                chartRoute(&pstShip->stMap, pstShip->psLocation, apsFields[4], apsFields[5], atoi(apsFields[6]));
            } else
                printString("Error: the island sent a wrong route.\n");
        }
    }

    if (0 == nError) {
        pstShip->nGold -= MAP_PRICE;
        if (-1 != asprintf(&psMessage, "Bought the map of %s for %d gold.\nNew routes have been charted.\n",
                           pstShip->psLocation, MAP_PRICE)) {
            printString(psMessage);
            free(psMessage);
        }
    }
}
