#define _GNU_SOURCE
#include "ship.h"
#include "voyages.h"
#include "sailing.h"
#include "market.h"

static void showStatus(Ship *pstShip) {
    OdysseusConfig *pstConfig = pstShip->pstConfig;
    char *psMessage = NULL;
    char *psName = copyText(pstConfig->psName);
    int nTotal = 0;
    int i = 0;

    for (i = 0; NULL != psName && '\0' != psName[i]; i++)
        psName[i] = toupper((unsigned char) psName[i]);
    if (NULL != psName && -1 != asprintf(&psMessage, "--- %s ---\nGold: %d\n\nCargo:\n", psName, pstShip->nGold)) {
        printString(psMessage);
        free(psMessage);
    }
    free(psName);

    for (i = 0; i < pstConfig->nNumCargo; i++) {
        if (0 < pstConfig->pstCargo[i].nKg &&
            -1 != asprintf(&psMessage, "%s: %d kg\n", pstConfig->pstCargo[i].psName, pstConfig->pstCargo[i].nKg)) {
            printString(psMessage);
            free(psMessage);
        }
        nTotal += pstConfig->pstCargo[i].nKg;
    }

    if (0 == pstShip->nVoyageId) {
        if (-1 != asprintf(&psMessage, "\nTotal provisions: %d kg\n\nCurrent voyage: None\nCurrent location: %s\n",
                           nTotal, pstShip->psLocation)) {
            printString(psMessage);
            free(psMessage);
        }
    } else if (-1 != asprintf(&psMessage, "\nTotal provisions: %d kg\n\nCurrent voyage: %s -> %s\nReward: %d\nCurrent location: %s\n",
                              nTotal, pstShip->psObject, pstShip->psDestination, pstShip->nReward, pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
}

void closeConnection(Ship *pstShip) {
    if (-1 != pstShip->nFd) {
        close(pstShip->nFd);
        pstShip->nFd = -1;
    }
}

void initShip(Ship *pstShip, OdysseusConfig *pstConfig) {
    pstShip->pstConfig = pstConfig;
    pstShip->nState = SHIP_AT_ITHACA;
    pstShip->nFd = -1;
    pstShip->psLocation = ITHACA_NAME;
    pstShip->nGold = pstConfig->nGold;
    pstShip->nVoyageId = 0;
    pstShip->psObject = NULL;
    pstShip->psDestination = NULL;
    pstShip->nReward = 0;
    pstShip->pnListedIds = NULL;
    pstShip->nNumListed = 0;
    initMap(&pstShip->stMap);
    // voyages begins there
    if (NULL == addIsland(&pstShip->stMap, pstConfig->psAeaeaName, pstConfig->psAeaeaIp, pstConfig->nAeaeaPort))
        printString("Error: not enough memory\n");
}

void freeShip(Ship *pstShip) {
    free(pstShip->psObject);
    free(pstShip->psDestination);
    forgetListedVoyages(pstShip);
    freeMap(&pstShip->stMap);
}

void executeCommand(Ship *pstShip, Command *pstCommand) {
    switch (pstCommand->nType) {
        case CMD_CONNECT_ITHACA:
            connectIthaca(pstShip);
            break;
        case CMD_LIST_VOYAGES:
            listVoyages(pstShip);
            break;
        case CMD_STATUS:
            showStatus(pstShip);
            break;
        case CMD_ACCEPT:
            acceptVoyage(pstShip, pstCommand->nNumber);
            break;
        case CMD_SAIL:
            sail(pstShip, pstCommand->psArgument);
            break;
        case CMD_LIST_MARKET:
            listMarket(pstShip);
            break;
        case CMD_BUY:
            buy(pstShip, pstCommand->psArgument, pstCommand->nNumber);
            break;
        case CMD_SELL:
            sell(pstShip, pstCommand->psArgument, pstCommand->nNumber);
            break;
        case CMD_BUY_MAP:
            buyMap(pstShip);
            break;
        case CMD_MAP:
            showMap(&pstShip->stMap);
            break;
        default:
            // Implemented by the next steps of the phase
            printString("Command OK\n");
            break;
    }
}