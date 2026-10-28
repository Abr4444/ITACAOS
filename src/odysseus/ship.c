#define _GNU_SOURCE
#include "ship.h"

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
}

void executeCommand(Ship *pstShip, Command *pstCommand) {
    switch (pstCommand->nType) {
        case CMD_CONNECT_ITHACA:
            connectIthaca(pstShip);
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