/************************************************
 *
 * @File: islandServer.c
 * @Purpose: An island's server: serves each Odysseus in its own thread,
 *           with the berths of the port, the FIFO queue of ships waiting
 *           offshore and the market (kept in stock.db).
 *           Every request is handled entirely inside stMutex: requests
 *           are short, and a ship waiting offshore blocks in read()
 *           outside the lock, so nobody is stalled.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "islandServer.h"

#include <limits.h>
#include <strings.h>

/************************************************
 *
 * @Name: islandOf
 * @Def: Gives the island that serves a ship.
 * @Arg: In: pstShip = the ship
 * @Ret: The IslandServer stored in the ship's pOwner.
 *
 ************************************************/
static IslandServer *islandOf(Client *pstShip) {

    return (IslandServer *) pstShip->pOwner;
}

/************************************************
 *
 * @Name: toAmount
 * @Def: Converts a positive amount of 1 to 9 digits.
 * @Arg: In: psWord = the amount as text
 * @Ret: The amount, or -1 if it is not a positive number.
 *
 ************************************************/
static int toAmount(char *psWord) {

    int nValue = 0;
    int i = 0;

    for (i = 0; '\0' != psWord[i]; i++) {
        if (9 <= i || '0' > psWord[i] || '9' < psWord[i]) {
            return -1;
        }
        nValue = nValue * 10 + (psWord[i] - '0');
    }
    return (0 == nValue) ? -1 : nValue;
}

/************************************************
 *
 * @Name: dockQueued
 * @Def: Gives the free berths to the queued ships with the lowest
 *       ticket (FIFO). Must be called with stMutex locked.
 * @Arg: In/Out: pstIsland = the island
 * @Ret: -
 *
 ************************************************/
static void dockQueued(IslandServer *pstIsland) {

    ClientList *pstShips = &pstIsland->stShips;
    Client *pstNext = NULL;
    int i = 0;

    while (pstIsland->nDocked < pstIsland->pstConfig->nCapacity) {
        pstNext = NULL;
        for (i = 0; i < pstShips->nNumClients; i++) {
            if (SHIP_QUEUED == pstShips->ppstClients[i]->nState &&
                (NULL == pstNext || pstShips->ppstClients[i]->nTicket < pstNext->nTicket)) {
                pstNext = pstShips->ppstClients[i];
            }
        }
        if (NULL == pstNext) {
            return;
        }
        pstNext->nState = SHIP_DOCKED;
        pstIsland->nDocked++;
        sendFrame(pstNext->nFd, TYPE_ARRIVAL, FLAGS_OK, "DOCKED");
        printClientEvent(pstNext->psName, "left the queue and docked.");
    }
}

/************************************************
 *
 * @Name: freeBerth
 * @Def: Departure, crash or lost at sea: frees the ship's berth or its
 *       place in the queue, and docks the next waiting ship.
 *       Must be called with stMutex locked.
 * @Arg: In/Out: pstShip = the ship that leaves
 * @Ret: -
 *
 ************************************************/
static void freeBerth(Client *pstShip) {

    IslandServer *pstIsland = islandOf(pstShip);

    if (SHIP_DOCKED == pstShip->nState) {
        pstIsland->nDocked--;
    }
    pstShip->nState = SHIP_OUTSIDE;
    if (0 == pstIsland->stShips.nClosing) {
        dockQueued(pstIsland);
    }
}

/************************************************
 *
 * @Name: handleArrival
 * @Def: ARRIVAL: docks the ship if there is a free berth, otherwise
 *       puts it at the end of the FIFO queue.
 * @Arg: In/Out: pstShip = the ship, gets its name and ticket
 *       In: pstFrame = the request, with the ship's name
 * @Ret: -
 *
 ************************************************/
static void handleArrival(Client *pstShip, Frame *pstFrame) {

    IslandServer *pstIsland = islandOf(pstShip);

    if (0 == pstFrame->nLength) {
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_FAIL, "EMPTY_NAME");
        return;
    }
    if (SHIP_OUTSIDE != pstShip->nState) {
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_FAIL, "ALREADY_ARRIVED");
        return;
    }
    free(pstShip->psName);
    pstShip->psName = malloc(pstFrame->nLength + 1);
    if (NULL == pstShip->psName) {
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_FAIL, "SERVER_OUT_OF_MEMORY");
        return;
    }
    strcpy(pstShip->psName, pstFrame->sData);

    pstShip->nTicket = pstIsland->nNextTicket++;
    if (pstIsland->nDocked < pstIsland->pstConfig->nCapacity) {
        pstShip->nState = SHIP_DOCKED;
        pstIsland->nDocked++;
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_OK, "DOCKED");
        printClientEvent(pstShip->psName, "arrived and docked.");
    } else {
        pstShip->nState = SHIP_QUEUED;
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_OK, "WAIT");
        printClientEvent(pstShip->psName, "arrived, the port is full: waiting offshore.");
    }
}

/************************************************
 *
 * @Name: handleMarket
 * @Def: LIST MARKET: one frame per product and a last one for the map.
 * @Arg: In: pstShip = the ship that asked
 * @Ret: -
 *
 ************************************************/
static void handleMarket(Client *pstShip) {

    IslandConfig *pstConfig = islandOf(pstShip)->pstConfig;
    char sData[FRAME_DATA_SIZE + 1];
    int nTotal = pstConfig->nNumProducts + 1;
    int i = 0;

    for (i = 0; i < pstConfig->nNumProducts; i++) {
        snprintf(sData, sizeof(sData), "%d&%d&%s&%d&%d", i + 1, nTotal, pstConfig->pstProducts[i].sName,
                 pstConfig->pstProducts[i].nAmount, pstConfig->pstProducts[i].nPrice);
        sendFrame(pstShip->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData);
    }
    snprintf(sData, sizeof(sData), "%d&%d&MAP&1&%d", nTotal, nTotal, MAP_PRICE);
    sendFrame(pstShip->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData);
}

/************************************************
 *
 * @Name: saveProduct
 * @Def: Writes product i in its place of stock.db (the records are
 *       consecutive, in the same order as in memory).
 * @Arg: In: pstIsland = the island
 *       In: i = index of the product
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
static int saveProduct(IslandServer *pstIsland, int i) {

    Product *pstProduct = &pstIsland->pstConfig->pstProducts[i];
    int nSize = sizeof(Product);

    if (-1 == lseek(pstIsland->nStockFd, i * nSize, SEEK_SET)) {
        return -1;
    }
    return (nSize == write(pstIsland->nStockFd, pstProduct, nSize)) ? 0 : -1;
}

/************************************************
 *
 * @Name: trade
 * @Def: BUY or SELL "<Product>&<Amount>". The gold is checked by
 *       Odysseus, the island only knows the prices.
 * @Arg: In: pstShip = the ship that trades
 *       In: pstFrame = the request
 *       Out: psAnswer = "OK&<Product>&<Amount>&<Cost or Income>"
 * @Ret: NULL on success, or the reason of the error.
 *
 ************************************************/
static char *trade(Client *pstShip, Frame *pstFrame, char *psAnswer) {

    IslandServer *pstIsland = islandOf(pstShip);
    IslandConfig *pstConfig = pstIsland->pstConfig;
    Product *pstProduct = NULL;
    char *psAmount = strchr(pstFrame->sData, '&');
    int nAmount = 0;
    int nDelta = 0;
    int i = 0;

    if (NULL == psAmount) {
        return NACK_INVALID_DATA;
    }
    *psAmount = '\0';
    nAmount = toAmount(psAmount + 1);
    if (-1 == nAmount) {
        return "INVALID_AMOUNT";
    }

    while (i < pstConfig->nNumProducts && 0 != strcasecmp(pstConfig->pstProducts[i].sName, pstFrame->sData)) {
        i++;
    }
    if (i == pstConfig->nNumProducts) {
        return "UNKNOWN_PRODUCT";
    }
    pstProduct = &pstConfig->pstProducts[i];
    if (TYPE_BUY == pstFrame->nType && nAmount > pstProduct->nAmount) {
        return "NOT_ENOUGH_STOCK";
    }
    if (TYPE_SELL == pstFrame->nType && nAmount > INT_MAX - pstProduct->nAmount) {
        return "STOCK_FULL";
    }

    nDelta = (TYPE_BUY == pstFrame->nType) ? -nAmount : nAmount;
    pstProduct->nAmount += nDelta;
    if (-1 == saveProduct(pstIsland, i)) {
        pstProduct->nAmount -= nDelta;   // keep memory and file equal
        return "STOCK_ERROR";
    }
    snprintf(psAnswer, FRAME_DATA_SIZE + 1, "OK&%s&%d&%lld", pstProduct->sName, nAmount,
             (long long) nAmount * pstProduct->nPrice);
    return NULL;
}

/************************************************
 *
 * @Name: handleMap
 * @Def: BUY MAP: sends one frame per route of this island.
 * @Arg: In: pstShip = the ship that buys it
 *       In: pstFrame = the request, with the amount (must be 1)
 * @Ret: -
 *
 ************************************************/
static void handleMap(Client *pstShip, Frame *pstFrame) {

    IslandConfig *pstConfig = islandOf(pstShip)->pstConfig;
    char sData[FRAME_DATA_SIZE + 1];
    int i = 0;

    if (1 != toAmount(pstFrame->sData)) {
        sendFrame(pstShip->nFd, TYPE_BUY_MAP, FLAGS_FAIL, "INVALID_AMOUNT");
        return;
    }
    if (0 == pstConfig->nNumRoutes) {
        snprintf(sData, sizeof(sData), "OK&%d&0", MAP_PRICE);
        sendFrame(pstShip->nFd, TYPE_BUY_MAP, FLAGS_OK, sData);
    }
    for (i = 0; i < pstConfig->nNumRoutes; i++) {
        snprintf(sData, sizeof(sData), "OK&%d&%d&%d&%s&%s&%d", MAP_PRICE, i + 1, pstConfig->nNumRoutes,
                 pstConfig->pstRoutes[i].psName, pstConfig->pstRoutes[i].psIp, pstConfig->pstRoutes[i].nPort);
        sendFrame(pstShip->nFd, TYPE_BUY_MAP, FLAGS_OK, sData);
    }
    printClientEvent(pstShip->psName, "bought the map.");
}

/************************************************
 *
 * @Name: handleRequest
 * @Def: Answers one request of a ship. Bad frames get a NACK and the
 *       ship keeps being served. Must be called with stMutex locked.
 * @Arg: In/Out: pstShip = the ship that sent it
 *       In: pstFrame = the request
 * @Ret: -
 *
 ************************************************/
static void handleRequest(Client *pstShip, Frame *pstFrame) {

    char sAnswer[FRAME_DATA_SIZE + 1];
    char *psReason = checkFrame(pstFrame);
    int nType = pstFrame->nType;

    if (NULL == psReason && (TYPE_ARRIVAL > nType || TYPE_DEPARTURE < nType)) {
        psReason = NACK_INVALID_TYPE;
    }
    if (NULL == psReason && FLAGS_REQUEST != pstFrame->nFlags) {
        psReason = NACK_INVALID_FLAGS;
    }

    if (NULL != psReason) {
        sendNack(pstShip->nFd, psReason);
    } else if (TYPE_ARRIVAL == nType) {
        handleArrival(pstShip, pstFrame);
    } else if (SHIP_DOCKED != pstShip->nState) {
        sendFrame(pstShip->nFd, nType, FLAGS_FAIL, "NOT_DOCKED");
    } else if (TYPE_LIST_MARKET == nType) {
        handleMarket(pstShip);
    } else if (TYPE_BUY == nType || TYPE_SELL == nType) {
        psReason = trade(pstShip, pstFrame, sAnswer);
        sendFrame(pstShip->nFd, nType, (NULL == psReason) ? FLAGS_OK : FLAGS_FAIL,
                  (NULL == psReason) ? sAnswer : psReason);
    } else if (TYPE_BUY_MAP == nType) {
        handleMap(pstShip, pstFrame);
    } else if (TYPE_DEPARTURE == nType) {
        freeBerth(pstShip);
        sendFrame(pstShip->nFd, TYPE_DEPARTURE, FLAGS_OK, "OK");
        printClientEvent(pstShip->psName, "left the port.");
    } else {
        // DELIVER is Phase 3
        sendFrame(pstShip->nFd, nType, FLAGS_FAIL, "NOT_AVAILABLE");
    }
}

/************************************************
 *
 * @Name: serveShip
 * @Def: Thread of one ship: answers its requests until it leaves, its
 *       connection breaks, or the island closes. Then frees its berth or
 *       its place in the queue, so a crashed ship never keeps one.
 * @Arg: In: pArg = the Client to serve
 * @Ret: NULL.
 *
 ************************************************/
static void *serveShip(void *pArg) {

    Client *pstShip = (Client *) pArg;
    ClientList *pstShips = &islandOf(pstShip)->stShips;
    Frame stFrame;

    while (0 == receiveFrame(pstShip->nFd, &stFrame)) {
        pthread_mutex_lock(&pstShips->stMutex);
        handleRequest(pstShip, &stFrame);
        pthread_mutex_unlock(&pstShips->stMutex);
    }

    pthread_mutex_lock(&pstShips->stMutex);
    // When the island itself is closing, the lost connections are expected
    if (SHIP_OUTSIDE != pstShip->nState && 0 == pstShips->nClosing) {
        printClientEvent(pstShip->psName, "lost the connection.");
    }
    freeBerth(pstShip);
    pstShip->nFinished = 1;
    pthread_mutex_unlock(&pstShips->stMutex);
    return NULL;
}

/************************************************
 *
 * @Name: initIslandServer
 * @Def: Prepares the island's server with an empty port and opens
 *       stock.db to save every change of the market.
 * @Arg: Out: pstIsland = the island
 *       In: pstConfig = the loaded island configuration and stock
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
int initIslandServer(IslandServer *pstIsland, IslandConfig *pstConfig) {

    pstIsland->pstConfig = pstConfig;
    pstIsland->nDocked = 0;
    pstIsland->nNextTicket = 0;

    pstIsland->nStockFd = open(pstConfig->psStockPath, O_WRONLY);
    if (-1 == pstIsland->nStockFd) {
        printError("Error: could not open the stock file for writing.\n");
        return -1;
    }
    if (-1 == initClientList(&pstIsland->stShips)) {
        close(pstIsland->nStockFd);
        return -1;
    }
    return 0;
}

/************************************************
 *
 * @Name: runIslandServer
 * @Def: Accepts ship connections, each one served by its own thread,
 *       until CTRL+C shuts the listening socket down.
 * @Arg: In/Out: pstIsland = the island
 *       In: nListenFd = listening socket
 * @Ret: -
 *
 ************************************************/
void runIslandServer(IslandServer *pstIsland, int nListenFd) {

    int nFd = acceptClient(nListenFd);

    while (-1 != nFd) {
        reapClients(&pstIsland->stShips);
        if (-1 == addClient(&pstIsland->stShips, nFd, serveShip, pstIsland)) {
            printError("Error: could not start serving a new Odysseus.\n");
            close(nFd);
        }
        nFd = acceptClient(nListenFd);
    }
    // EINVAL means CTRL+C shut the socket down, which is the normal exit.
    // Any other errno means accept broke on its own, so say why we stop
    if (EINVAL != errno) {
        printError("Error: the island stopped accepting ships unexpectedly.\n");
    }
}

/************************************************
 *
 * @Name: closeIslandServer
 * @Def: Closes every ship connection, frees the server and closes
 *       stock.db.
 * @Arg: In/Out: pstIsland = the island
 * @Ret: -
 *
 ************************************************/
void closeIslandServer(IslandServer *pstIsland) {

    closeClientList(&pstIsland->stShips);
    close(pstIsland->nStockFd);
}
