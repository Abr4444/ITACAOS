/************************************************
 * @File: delivery.c
 * @Purpose: The end of a voyage: DELIVER at the destination island
 *           and CLAIM at Ithaca.
 * @Date: 21/11/2026
 ************************************************/

#define _GNU_SOURCE
#include "delivery.h"

/* @Name: objectOnBoard
 * @Def: Checks that the object can be opened for reading.
 * @Arg: In: psPath = path of the object
 * @Ret: 1 if it is on board, 0 otherwise. */
static int objectOnBoard(char *psPath) {
    int nFd = open(psPath, O_RDONLY);

    if (-1 == nFd)
        return 0;
    close(nFd);
    return 1;
}

/************************************************
 * @Name: canDeliver
 * @Def: DELIVER needs an active voyage whose object is not delivered yet
 *       and still on board, and the ship docked at its destination.
 * @Arg: In: pstShip = the ship
 * @Ret: 1 if it can deliver, 0 otherwise (the reason is printed).
 ************************************************/
static int canDeliver(Ship *pstShip) {
    char *psMessage = NULL;

    if (0 == pstShip->nVoyageId) {
        printString("You have no active voyage.\n");
        return 0;
    }
    if (1 == pstShip->nDelivered) {
        printString("The object has already been delivered.\n");
        return 0;
    }
    if (SHIP_DOCKED != pstShip->nState || 0 != strcasecmp(pstShip->psLocation, pstShip->psDestination)) {
        if (-1 != asprintf(&psMessage, "You must be docked at %s to deliver the object.\n", pstShip->psDestination)) {
            printString(psMessage);
            free(psMessage);
        }
        return 0;
    }
    if (0 == objectOnBoard(pstShip->psObjectPath)) {
        printString("Error: the object is no longer on board.\n");
        return 0;
    }
    return 1;
}
/************************************************
 * @Name: receiveStamped
 * @Def: SIGNED: receives the stamped image (signed_<name>) and only then
 *       renames it over the original, so a failed transfer keeps the
 *       original on board and DELIVER can be tried again.
 * @Arg: In/Out: pstShip = the ship
 *       In: pstHeader = the SIGNED header already received
 * @Ret: -
 ************************************************/
static void receiveStamped(Ship *pstShip, Frame *pstHeader) {
    char *psPath = receiveFile(pstShip->nFd, pstHeader, "SIGNED", pstShip->pstConfig->psFolder);

    if (NULL == psPath)
        return;
    rename(psPath, pstShip->psObjectPath);
    free(psPath);
    printString("\nStamped object received.\n");
}
/************************************************
 * @Name: deliver
 * @Def: DELIVER: gives the object to the destination island, which stamps
 *       it and sends it back.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 ************************************************/
void deliver(Ship *pstShip) {
    Frame stFrame;
    char *psMessage = NULL;
    int nSent = -1;
    int nTransfer = -1;

    if (0 == canDeliver(pstShip))
        return;
    if (-1 != asprintf(&psMessage, "%d&%s", pstShip->nVoyageId, pstShip->psObject)) {
        nSent = sendFrame(pstShip->nFd, TYPE_DELIVER, FLAGS_REQUEST, psMessage);
        free(psMessage);
    }
    if (-1 != asprintf(&psMessage, "Delivering %s to %s...\n", pstShip->psObject, pstShip->psDestination)) {
        printString(psMessage);
        free(psMessage);
    }
    if (-1 != nSent)
        nTransfer = sendFile(pstShip->nFd, "DELIVERY", pstShip->psObjectPath, fileNameOf(pstShip->psObjectPath), FLAGS_REQUEST);
    if (0 == nTransfer && -1 != asprintf(&psMessage, "Object transferred correctly.\n\n%s is stamping the object...\n", pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
    if (0 != nTransfer && 1 == *pstShip->pnStop) {
        closeConnection(pstShip);
        return;
    }
    if (-1 == nSent || -1 == receiveFrame(pstShip->nFd, &stFrame)) {
        lostIsland(pstShip);
        return;
    }
    if (TYPE_FILE_HEADER == stFrame.nType) {
        receiveStamped(pstShip, &stFrame);
        if (-1 == receiveFrame(pstShip->nFd, &stFrame)) {
            lostIsland(pstShip);
            return;
        }
    }
    if (TYPE_DELIVER != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "The delivery failed: %s\n", stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    pstShip->nDelivered = 1;
    if (-1 != asprintf(&psMessage, "\nDelivery completed.\nThe mark of %s has been obtained.\n", pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
}
/************************************************
 * @Name: canClaim
 * @Def: CLAIM needs the connection with Ithaca and a delivered object
 *       still on board.
 * @Arg: In: pstShip = the ship
 * @Ret: 1 if it can claim, 0 otherwise (the reason is printed).
 ************************************************/
static int canClaim(Ship *pstShip) {
    if (SHIP_CONNECTED_ITHACA != pstShip->nState) {
        printString("You must be connected to Ithaca first.\n");
        return 0;
    }
    if (0 == pstShip->nVoyageId || 0 == pstShip->nDelivered) {
        printString("You have no delivered object to claim.\n");
        return 0;
    }
    if (0 == objectOnBoard(pstShip->psObjectPath)) {
        printString("Error: the stamped object is no longer on board.\n");
        return 0;
    }
    return 1;
}

/************************************************
 * @Name: claim
 * @Def: CLAIM: sends the stamped image to Ithaca, which checks it and pays the reward.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: -
 ************************************************/
void claim(Ship *pstShip) {
    Frame stFrame;
    char *apsFields[CLAIM_FIELDS];
    char *psMessage = NULL;
    int nSent = -1;
    int nTransfer = -1;

    if (0 == canClaim(pstShip))
        return;
    if (-1 != asprintf(&psMessage, "%d", pstShip->nVoyageId)) {
        nSent = sendFrame(pstShip->nFd, TYPE_CLAIM, FLAGS_REQUEST, psMessage);
        free(psMessage);
    }
    if (-1 != asprintf(&psMessage, "Presenting the stamped %s...\n", pstShip->psObject)) {
        printString(psMessage);
        free(psMessage);
    }
    if (-1 != nSent)
        nTransfer = sendFile(pstShip->nFd, "CLAIM", pstShip->psObjectPath, fileNameOf(pstShip->psObjectPath), FLAGS_REQUEST);
    if (0 == nTransfer)
        printString("\nObject transferred correctly.\n");
    if (0 != nTransfer && 1 == *pstShip->pnStop) {
        closeConnection(pstShip);
        return;
    }
    if (-1 == nSent || -1 == receiveFrame(pstShip->nFd, &stFrame)) {
        lostIthaca(pstShip);
        return;
    }
    if (TYPE_CLAIM != stFrame.nType || FLAGS_OK != stFrame.nFlags) {
        if (-1 != asprintf(&psMessage, "Ithaca rejected the claim: %s\n", stFrame.sData)) {
            printString(psMessage);
            free(psMessage);
        }
        return;
    }
    if (CLAIM_FIELDS != splitFields(stFrame.sData, apsFields, CLAIM_FIELDS)) {
        printString("Error: Ithaca sent a wrong answer.\n");
        return;
    }
    pstShip->nGold += atoi(apsFields[1]);
    if (-1 != asprintf(&psMessage, "Glyph of %s verified.\n\nVoyage %d completed.\nReward received: %s gold.\n",
                       pstShip->psDestination, pstShip->nVoyageId, apsFields[1])) {
        printString(psMessage);
        free(psMessage);
    }
    dropVoyage(pstShip);
}