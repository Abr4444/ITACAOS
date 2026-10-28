#define _GNU_SOURCE
#include "islandServer.h"

#include <limits.h>
#include <strings.h>

static IslandServer *islandOf(Client *pstShip) {
    return (IslandServer *) pstShip->pstServer->pData;
}

// Every request is handled entirely inside stMutex: requests are short, and a
// ship waiting offshore blocks in read() outside the lock, so nobody is stalled.

// positive number of 1-9 digits, -1 otherwise
static int toAmount(char *psWord) {
    int nValue = 0, i = 0;

    for (i = 0; '\0' != psWord[i]; i++) {
        if (9 <= i || '0' > psWord[i] || '9' < psWord[i])
            return -1;
        nValue = nValue * 10 + (psWord[i] - '0');
    }
    return (0 == nValue) ? -1 : nValue;
}

// gives the free berths to the queued ships with the lowest ticket (FIFO)
static void dockQueued(IslandServer *pstIsland) {
    Client *pstNext = NULL;
    int i = 0;

    while (pstIsland->nDocked < pstIsland->pstConfig->nCapacity) {
        pstNext = NULL;
        for (i = 0; i < pstIsland->stServer.nNumClients; i++) {
            if (SHIP_QUEUED == pstIsland->stServer.ppstClients[i]->nState &&
                (NULL == pstNext || pstIsland->stServer.ppstClients[i]->nTicket < pstNext->nTicket))
                pstNext = pstIsland->stServer.ppstClients[i];
        }
        if (NULL == pstNext)
            return;
        pstNext->nState = SHIP_DOCKED;
        pstIsland->nDocked++;
        sendFrame(pstNext->nFd, TYPE_ARRIVAL, FLAGS_OK, "DOCKED");
        printClientEvent(pstNext->psName, "left the queue and docked.");
    }
}

// departure, crash or lost at sea: frees the berth / place in the queue
static void freeBerth(Client *pstShip) {
    IslandServer *pstIsland = islandOf(pstShip);

    if (SHIP_DOCKED == pstShip->nState)
        pstIsland->nDocked--;
    pstShip->nState = SHIP_OUTSIDE;
    if (0 == pstIsland->stServer.nClosing)
        dockQueued(pstIsland);
}

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
    pstShip->psName = strdup(pstFrame->sData);
    if (NULL == pstShip->psName) {
        sendFrame(pstShip->nFd, TYPE_ARRIVAL, FLAGS_FAIL, "SERVER_OUT_OF_MEMORY");
        return;
    }
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

// one frame per product and a last one for the map
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

// writes product i in its place of stock.db (records are consecutive)
static int saveProduct(IslandServer *pstIsland, int i) {
    Product *pstProduct = &pstIsland->pstConfig->pstProducts[i];

    if (-1 == lseek(pstIsland->nStockFd, (off_t) i * sizeof(Product), SEEK_SET))
        return -1;
    return ((ssize_t) sizeof(Product) == write(pstIsland->nStockFd, pstProduct, sizeof(Product))) ? 0 : -1;
}

// BUY / SELL "<Product>&<Amount>". The gold is checked by Odysseus, the island only knows prices.
static char *trade(Client *pstShip, Frame *pstFrame, char *psAnswer) {
    IslandServer *pstIsland = islandOf(pstShip);
    IslandConfig *pstConfig = pstIsland->pstConfig;
    Product *pstProduct = NULL;
    char *psAmount = strchr(pstFrame->sData, '&');
    int nAmount = 0, nDelta = 0, i = 0;

    if (NULL == psAmount)
        return NACK_INVALID_DATA;
    *psAmount = '\0';
    nAmount = toAmount(psAmount + 1);
    if (-1 == nAmount)
        return "INVALID_AMOUNT";

    while (i < pstConfig->nNumProducts && 0 != strcasecmp(pstConfig->pstProducts[i].sName, pstFrame->sData))
        i++;
    if (i == pstConfig->nNumProducts)
        return "UNKNOWN_PRODUCT";
    pstProduct = &pstConfig->pstProducts[i];
    if (TYPE_BUY == pstFrame->nType && nAmount > pstProduct->nAmount)
        return "NOT_ENOUGH_STOCK";
    if (TYPE_SELL == pstFrame->nType && nAmount > INT_MAX - pstProduct->nAmount)
        return "STOCK_FULL";

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

// mutex held. Bad frames get a NACK and the ship keeps being served
static void handleRequest(Client *pstShip, Frame *pstFrame) {
    char sAnswer[FRAME_DATA_SIZE + 1];
    char *psReason = checkFrame(pstFrame);
    int nType = pstFrame->nType;

    if (NULL == psReason && (TYPE_ARRIVAL > nType || TYPE_DEPARTURE < nType))
        psReason = NACK_INVALID_TYPE;
    if (NULL == psReason && FLAGS_REQUEST != pstFrame->nFlags)
        psReason = NACK_INVALID_FLAGS;

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
    } else {    // DELIVER is Phase 3
        sendFrame(pstShip->nFd, nType, FLAGS_FAIL, "NOT_AVAILABLE");
    }
}

// runs in the ship's thread (called by server.c). Each request is handled inside stMutex
static void serveShip(Client *pstShip) {
    pthread_mutex_t *pstMutex = &pstShip->pstServer->stMutex;
    Frame stFrame;

    while (0 == receiveFrame(pstShip->nFd, &stFrame)) {
        pthread_mutex_lock(pstMutex);
        handleRequest(pstShip, &stFrame);
        pthread_mutex_unlock(pstMutex);
    }
}

// stMutex held (called by server.c): a crashed / lost ship never keeps a berth
static void endShip(Client *pstShip) {
    if (SHIP_OUTSIDE != pstShip->nState && 0 == pstShip->pstServer->nClosing)
        printClientEvent(pstShip->psName, "lost the connection.");
    freeBerth(pstShip);
}

int initIslandServer(IslandServer *pstIsland, IslandConfig *pstConfig) {
    memset(pstIsland, 0, sizeof(IslandServer));
    pstIsland->pstConfig = pstConfig;

    pstIsland->nStockFd = open(pstConfig->psStockPath, O_WRONLY);
    if (-1 == pstIsland->nStockFd) {
        printError("Error: could not open the stock file for writing.\n");
        return -1;
    }
    if (-1 == initServer(&pstIsland->stServer, pstIsland, serveShip, endShip)) {
        close(pstIsland->nStockFd);
        return -1;
    }
    return 0;
}

void closeIslandServer(IslandServer *pstIsland) {
    closeServer(&pstIsland->stServer);
    close(pstIsland->nStockFd);
}
