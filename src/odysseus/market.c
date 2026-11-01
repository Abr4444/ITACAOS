#define _GNU_SOURCE
#include "market.h"

/* @Name: lostIsland
 * @Def: The island stopped answering: close the connection.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
static void lostIsland(Ship *pstShip) {

    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, "Error: lost the connection with %s.\n", pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
    closeConnection(pstShip);
    pstShip->nState = SHIP_AT_SEA;
}


/* @Name: isDocked
 * @Def: Market commands only work while docked.
 * @Arg: In: pstShip = the ship
 * @Ret: 1 if docked, 0 otherwise */
static int isDocked(Ship *pstShip) {

    if (SHIP_DOCKED != pstShip->nState) {
        printString("You must be docked at an island first.\n");
        return 0;
    }
    return 1;
}

/* @Name: findCargo
 * @Def: Finds a food in the cargo (case insensitive).
 * @Arg: In: pstConfig = config with the cargo
 *       In: psName = food
 * @Ret: Its index, or -1 */
static int findCargo(OdysseusConfig *pstConfig, char *psName) {

    int i = 0;

    for (i = 0; i < pstConfig->nNumCargo; i++) {
        if (0 == strcasecmp(pstConfig->pstCargo[i].psName, psName))
            return i;
    }
    return -1;
}

/* @Name: addCargo
 * @Def: Adds bought food to the cargo (new entry if needed).
 * @Arg: In/Out: pstConfig = config with the cargo
 *       In: psName = food
 *       In: nKg = kg bought
 * @Ret: - */
static void addCargo(OdysseusConfig *pstConfig, char *psName, int nKg) {

    Cargo *pstMore = NULL;
    int i = findCargo(pstConfig, psName);

    if (-1 != i) {
        pstConfig->pstCargo[i].nKg += nKg;
        return;
    }
    pstMore = realloc(pstConfig->pstCargo, (pstConfig->nNumCargo + 1) * sizeof(Cargo));
    if (NULL == pstMore) {
        printString("Error: not enough memory for the cargo.\n");
        return;
    }
    pstConfig->pstCargo = pstMore;
    pstConfig->pstCargo[pstConfig->nNumCargo].psName = copyText(psName);
    if (NULL == pstConfig->pstCargo[pstConfig->nNumCargo].psName) {
        printString("Error: not enough memory for the cargo.\n");
        return;
    }
    pstConfig->pstCargo[pstConfig->nNumCargo].nKg = nKg;
    pstConfig->nNumCargo++;
}

/* @Name: listMarket
 * @Def: LIST MARKET: prints the island's products and the map.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
void listMarket(Ship *pstShip) {

    Frame stFrame;
    char *apsFields[MARKET_FIELDS];
    char *psName = NULL;
    char *psMessage = NULL;
    int nReceived = 0;
    int nTotal = 1;
    int i = 0;

    if (0 == isDocked(pstShip))
        return;
    if (-1 == sendFrame(pstShip->nFd, TYPE_LIST_MARKET, FLAGS_REQUEST, NULL)) {
        lostIsland(pstShip);
        return;
    }

    psName = copyText(pstShip->psLocation);
    for (i = 0; NULL != psName && '\0' != psName[i]; i++)
        psName[i] = toupper((unsigned char) psName[i]);
    if (NULL != psName && -1 != asprintf(&psMessage, "--- %s MARKET ---\n", psName)) {
        printString(psMessage);
        free(psMessage);
    }
    free(psName);

    while (nReceived < nTotal) {
        if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            lostIsland(pstShip);
            return;
        }
        nReceived++;
        if (FLAGS_OK != stFrame.nFlags || MARKET_FIELDS != splitFields(stFrame.sData, apsFields, MARKET_FIELDS)) {
            printString("Error: the island could not send its market.\n");
            return;
        }
        nTotal = atoi(apsFields[1]);
        if (0 == strcmp(apsFields[2], "MAP")) {
            if (-1 != asprintf(&psMessage, "%-16s%13s gold\n", "Map", apsFields[4])) {
                printString(psMessage);
                free(psMessage);
            }
        } else if (-1 != asprintf(&psMessage, "%-16s%4s kg%6s gold/kg\n", apsFields[2], apsFields[3], apsFields[4])) {
            printString(psMessage);
            free(psMessage);
        }
    }
}

/* @Name: getPrice
 * @Def: Asks the market (without printing) to get a price.
 * @Arg: In/Out: pstShip = the ship
 *       In: psProduct = food
 * @Ret: Its price, or -1 */
static int getPrice(Ship *pstShip, char *psProduct) {

    Frame stFrame;
    char *apsFields[MARKET_FIELDS];
    int nPrice = -1;
    int nReceived = 0;
    int nTotal = 1;

    if (-1 == sendFrame(pstShip->nFd, TYPE_LIST_MARKET, FLAGS_REQUEST, NULL)) {
        lostIsland(pstShip);
        return -1;
    }
    while (nReceived < nTotal) {
        if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            lostIsland(pstShip);
            return -1;
        }
        nReceived++;
        if (FLAGS_OK != stFrame.nFlags || MARKET_FIELDS != splitFields(stFrame.sData, apsFields, MARKET_FIELDS))
            return -1;
        nTotal = atoi(apsFields[1]);
        if (0 == strcasecmp(apsFields[2], psProduct))
            nPrice = atoi(apsFields[4]);
    }
    return nPrice;
}

/* @Name: buy
 * @Def: BUY: checks the price and gold, buys, adds to the cargo.
 * @Arg: In/Out: pstShip = the ship
 *       In: psProduct = food
 *       In: nAmount = kg
 * @Ret: - */
void buy(Ship *pstShip, char *psProduct, int nAmount) {

    Frame stFrame;
    char *apsFields[TRADE_FIELDS];
    char *psMessage = NULL;
    int nPrice = 0;
    int nSent = -1;

    if (0 == isDocked(pstShip))
        return;
    nPrice = getPrice(pstShip, psProduct);
    if (-1 == nPrice) {
        if (-1 != asprintf(&psMessage, "%s does not sell %s.\n", pstShip->psLocation, psProduct)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (nAmount * nPrice > pstShip->nGold) {
        if (-1 != asprintf(&psMessage, "You don't have enough gold: %d kg of %s cost %d gold.\n",
                           nAmount, psProduct, nAmount * nPrice)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }

    if (-1 != asprintf(&psMessage, "%s&%d", psProduct, nAmount)) {
        nSent = sendFrame(pstShip->nFd, TYPE_BUY, FLAGS_REQUEST, psMessage);
        free(psMessage);
    }
    if (-1 == nSent || -1 == receiveFrame(pstShip->nFd, &stFrame)) {
        lostIsland(pstShip);
        return;
    }
    if (TYPE_BUY != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "%s refused the purchase: %s\n", pstShip->psLocation, stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (TRADE_FIELDS != splitFields(stFrame.sData, apsFields, TRADE_FIELDS)) {
        printString("Error: the island sent a wrong answer.\n");
        return;
    }

    addCargo(pstShip->pstConfig, apsFields[1], atoi(apsFields[2]));
    pstShip->nGold -= atoi(apsFields[3]);
    if (-1 != asprintf(&psMessage, "Bought %s kg of %s for %s gold.\n", apsFields[2], apsFields[1], apsFields[3])) {
        printString(psMessage);
        free(psMessage);
    }
}

/* @Name: sell
 * @Def: SELL: checks the cargo, sells, adds the gold.
 * @Arg: In/Out: pstShip = the ship
 *       In: psProduct = food
 *       In: nAmount = kg
 * @Ret: - */
void sell(Ship *pstShip, char *psProduct, int nAmount) {

    OdysseusConfig *pstConfig = pstShip->pstConfig;
    Frame stFrame;
    char *apsFields[TRADE_FIELDS];
    char *psMessage = NULL;
    int nSent = -1;
    int i = 0;

    if (0 == isDocked(pstShip))
        return;
    i = findCargo(pstConfig, psProduct);
    if (-1 == i || nAmount > pstConfig->pstCargo[i].nKg) {
        if (-1 != asprintf(&psMessage, "You don't carry %d kg of %s.\n", nAmount, psProduct)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }

    if (-1 != asprintf(&psMessage, "%s&%d", pstConfig->pstCargo[i].psName, nAmount)) {
        nSent = sendFrame(pstShip->nFd, TYPE_SELL, FLAGS_REQUEST, psMessage);
        free(psMessage);
    }
    if (-1 == nSent || -1 == receiveFrame(pstShip->nFd, &stFrame)) {
        lostIsland(pstShip);
        return;
    }
    if (TYPE_SELL != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "%s refused the sale: %s\n", pstShip->psLocation, stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (TRADE_FIELDS != splitFields(stFrame.sData, apsFields, TRADE_FIELDS)) {
        printString("Error: the island sent a wrong answer.\n");
        return;
    }

    pstConfig->pstCargo[i].nKg -= nAmount;
    pstShip->nGold += atoi(apsFields[3]);
    if (-1 != asprintf(&psMessage, "Sold %d kg of %s for %s gold.\n", nAmount, pstConfig->pstCargo[i].psName, apsFields[3])) {
        printString(psMessage);
        free(psMessage);
    }
}

/* @Name: buyMap
 * @Def: BUY MAP 1: pays 50 gold and charts the island's routes.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
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
