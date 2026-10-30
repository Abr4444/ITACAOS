#define _GNU_SOURCE
#include "ship.h"
#define ITHACA_NAME "Ithaca"


static void showStatus(Ship *pstShip) {
    OdysseusConfig *pstConfig = pstShip->pstConfig;
    char *psMessage = NULL;
    char *psName = copyText(pstConfig->psName);
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

/************************************************
 *
 * @Name: lostIthaca
 * @Def: Ithaca stopped answering (it crashed or closed): the ship
 *       forgets the connection and stays in Ithaca's harbor.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 *
 ************************************************/
static void lostIthaca(Ship *pstShip) {
    printString("Error: lost the connection with Ithaca.\n");
    closeConnection(pstShip);
    pstShip->nState = SHIP_AT_ITHACA;
}

/************************************************
 *
 * @Name: forgetListedVoyages
 * @Def: Empties the IDs of the last LIST VOYAGES.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 *
 ************************************************/
static void forgetListedVoyages(Ship *pstShip) {

    free(pstShip->pnListedIds);
    pstShip->pnListedIds = NULL;
    pstShip->nNumListed = 0;
}

/************************************************
 *
 * @Name: canAskForVoyage
 * @Def: LIST VOYAGES and ACCEPT only work while connected to Ithaca and
 *       without an active voyage.
 * @Arg: In: pstShip = the ship
 * @Ret: 1 if the command can run, 0 otherwise (the reason is printed).
 *
 ************************************************/
static int canAskForVoyage(Ship *pstShip) {

    if (SHIP_CONNECTED_ITHACA != pstShip->nState) {
        printString("You must be connected to Ithaca first.\n");
        return 0;
    }
    if (0 != pstShip->nVoyageId) {
        printString("You already have an active voyage.\n");
        return 0;
    }
    return 1;
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

/************************************************
 *
 * @Name: rememberVoyageId
 * @Def: Adds a voyage ID to the list of the last LIST VOYAGES.
 * @Arg: In/Out: pstShip = the ship
 *       In: nId = the voyage ID
 * @Ret: -
 *
 ************************************************/
static void rememberVoyageId(Ship *pstShip, int nId) {

    int *pnMore = realloc(pstShip->pnListedIds, (pstShip->nNumListed + 1) * sizeof(int));

    if (NULL == pnMore) {
        printString("Error: not enough memory to remember the voyage list.\n");
        return;
    }
    pstShip->pnListedIds = pnMore;
    pstShip->pnListedIds[pstShip->nNumListed] = nId;
    pstShip->nNumListed++;
}

/************************************************
 *
 * @Name: listVoyages
 * @Def: LIST VOYAGES: asks Ithaca for the available voyages and prints
 *       them. Ithaca sends one frame per voyage ("Index&Total&...")
 *       or a single "0" if there are none.
 * @Arg: In/Out: pstShip = the ship, remembers the IDs listed
 * @Ret: -
 *
 ************************************************/
static void listVoyages(Ship *pstShip) {

    Frame stFrame;
    char *apsFields[LIST_VOYAGE_FIELDS];
    char *psMessage = NULL;
    int nReceived = 0;
    int nTotal = 1;     // until the first frame says how many there are
    int nError = 0;

    if (0 == canAskForVoyage(pstShip))
        return;
    if (-1 == sendFrame(pstShip->nFd, TYPE_LIST_VOYAGES, FLAGS_REQUEST, NULL)) {
        lostIthaca(pstShip);
        return;
    }

    forgetListedVoyages(pstShip);
    printString("--- AVAILABLE VOYAGES ---\n");
    while (nReceived < nTotal && 0 == nError) {
        if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            lostIthaca(pstShip);
            return;
        }
        nReceived++;
        if (TYPE_LIST_VOYAGES != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
            // Ithaca sends nothing more after an error
            printString("Error: Ithaca could not send the voyages.\n");
            nError = 1;
        } else if (0 == strcmp(stFrame.sData, "0")) {
            printString("No voyages available.\n");
            nTotal = 0;
        } else if (LIST_VOYAGE_FIELDS == splitFields(stFrame.sData, apsFields, LIST_VOYAGE_FIELDS)) {
            nTotal = atoi(apsFields[1]);
            rememberVoyageId(pstShip, atoi(apsFields[2]));
            if (-1 != asprintf(&psMessage, "%s. %-18s -> %-14s [%s]\n", apsFields[2], apsFields[3], apsFields[4], apsFields[5])) {
                printString(psMessage);
                free(psMessage);
            }
        } else {
            printString("Error: Ithaca sent a wrong voyage.\n");
        }
    }
}

/************************************************
 *
 * @Name: isListedVoyage
 * @Def: Checks if a voyage was in the last LIST VOYAGES.
 * @Arg: In: pstShip = the ship
 *       In: nId = the voyage ID
 * @Ret: 1 if it was listed, 0 otherwise.
 *
 ************************************************/
static int isListedVoyage(Ship *pstShip, int nId) {
    for (int i = 0; i < pstShip->nNumListed; i++)
        if (nId == pstShip->pnListedIds[i])
            return 1;
    return 0;
}

/************************************************
 *
 * @Name: acceptVoyage
 * @Def: ACCEPT id: asks Ithaca to assign a voyage of the last
 *       LIST VOYAGES to this ship, and stores it as the active voyage.
 * @Arg: In/Out: pstShip = the ship
 *       In: nId = the voyage ID
 * @Ret: -
 *
 ************************************************/
static void acceptVoyage(Ship *pstShip, int nId) {
    Frame stFrame;
    char *apsFields[ACCEPT_VOYAGE_FIELDS];
    char *psMessage = NULL;
    char *psId = NULL;
    int nSent = -1;

    if (0 == canAskForVoyage(pstShip))
        return;
    if (0 == isListedVoyage(pstShip, nId)) {
        if (-1 != asprintf(&psMessage, "Voyage %d is not in the last LIST VOYAGES.\n", nId)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }

    if (-1 != asprintf(&psId, "%d", nId)) {
        nSent = sendFrame(pstShip->nFd, TYPE_ACCEPT, FLAGS_REQUEST, psId);
        free(psId);
    }
    if (-1 == nSent || -1 == receiveFrame(pstShip->nFd, &stFrame)) {
        lostIthaca(pstShip);
        return;
    }
    if (TYPE_ACCEPT != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "Ithaca refused the voyage: %s\n", stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (ACCEPT_VOYAGE_FIELDS != splitFields(stFrame.sData, apsFields, ACCEPT_VOYAGE_FIELDS)) {
        printString("Error: Ithaca sent a wrong voyage.\n");
        return;
    }

    pstShip->psObject = copyText(apsFields[2]);
    pstShip->psDestination = copyText(apsFields[3]);
    if (NULL == pstShip->psObject || NULL == pstShip->psDestination) {
        printString("Error: not enough memory for voyage.\n");
        free(pstShip->psObject);
        free(pstShip->psDestination);
        pstShip->psObject = NULL;
        pstShip->psDestination = NULL;
        return;
    }
    pstShip->nVoyageId = atoi(apsFields[1]);
    pstShip->nReward = atoi(apsFields[4]);
    forgetListedVoyages(pstShip);

    if (-1 != asprintf(&psMessage, "Voyage %d accepted.\nDestination: %s.\n",
                       pstShip->nVoyageId, pstShip->psDestination)) {
        printString(psMessage);
        free(psMessage);
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
}

void freeShip(Ship *pstShip) {
    free(pstShip->psObject);
    free(pstShip->psDestination);
    forgetListedVoyages(pstShip);
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