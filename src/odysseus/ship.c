#define _GNU_SOURCE
#include "ship.h"
#define ITHACA_NAME "Ithaca"

static void showStatus(Ship *pstShip) {
    OdysseusConfig *pstConfig = pstShip->pstConfig;
    char *psMessage = NULL;
    char *psName = strdup(pstConfig->psName);
    int nTotal = 0;
    int i = 0;

    for (i = 0; NULL != psName && '\0' != psName[i]; i++)
        psName[i] = toupper((unsigned char) psName[i]);
    if (-1 != asprintf(&psMessage, "--- %s ---\nGold: %d\n\nCargo:\n",
                       (NULL != psName) ? psName : pstConfig->psName, pstShip->nGold)) {
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

static void closeConnection(Ship *pstShip) {

    if (-1 != pstShip->nFd) {
        close(pstShip->nFd);
        pstShip->nFd = -1;
    }
}

static void connectIthaca(Ship *pstShip) {

    OdysseusConfig *pstConfig = pstShip->pstConfig;
    Frame stFrame;
    char *psMessage = NULL;

    if (SHIP_CONNECTED_ITHACA == pstShip->nState) {
        printString("You are already connected to Ithaca.\n");
        return;
    }

    pstShip->nFd = openClientConn(pstConfig->psIthacaIp, pstConfig->nIthacaPort);
    if (-1 == pstShip->nFd) {
        printString("Error: could not reach Ithaca.\n");
        return;
    }
    if (-1 == sendFrame(pstShip->nFd, TYPE_CONNECT_ITHACA, FLAGS_REQUEST, pstConfig->psName) ||
        -1 == receiveFrame(pstShip->nFd, &stFrame) || TYPE_CONNECT_ITHACA != stFrame.nType) {
        printString("Error: Ithaca did not answer.\n");
        closeConnection(pstShip);
        return;
    }
    if (FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "Ithaca refused the connection: %s\n", stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        closeConnection(pstShip);
        return;
    }

    pstShip->nState = SHIP_CONNECTED_ITHACA;
    printString("Connected to Ithaca.\n");
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
}

void freeShip(Ship *pstShip) {
    free(pstShip->psObject);
    free(pstShip->psDestination);
}

void executeCommand(Ship *pstShip, Command *pstCommand) {
    switch (pstCommand->nType) {
        case CMD_CONNECT_ITHACA:
            connectIthaca(pstShip);
            break;
        case CMD_STATUS:
            showStatus(pstShip);
            break;
        default:
            // Implemented by the next steps of the phase
            printString("Command OK\n");
            break;
    }
}

void leavePort(Ship *pstShip) {

    Frame stFrame;

    if (SHIP_CONNECTED_ITHACA == pstShip->nState) {
        if (0 == sendFrame(pstShip->nFd, TYPE_DISCONNECT_ITHACA, FLAGS_REQUEST, NULL)) {
            receiveFrame(pstShip->nFd, &stFrame);
        }
        pstShip->nState = SHIP_AT_ITHACA;
    }
    closeConnection(pstShip);
}