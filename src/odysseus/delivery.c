/************************************************
 * @File: delivery.c
 * @Purpose: The end of a voyage: DELIVER at the destination island
 *           and CLAIM at Ithaca.
 * @Date: 21/11/2026
 ************************************************/

#define _GNU_SOURCE
#include "delivery.h"

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
    if (0 != access(pstShip->psObjectPath, R_OK)) {
        printString("Error: the object is no longer on board.\n");
        return 0;
    }
    return 1;
}
/************************************************
 * @Name: receiveStamped
 * @Def: SIGNED: receives the stamped image. It has the same name, so it
 *       replaces the original one on board.
 * @Arg: In/Out: pstShip = the ship
 *       In: pstHeader = the SIGNED header already received
 * @Ret: -
 ************************************************/
static void receiveStamped(Ship *pstShip, Frame *pstHeader) {
    char *psPath = receiveFile(pstShip->nFd, pstHeader, "SIGNED", pstShip->pstConfig->psFolder);

    if (NULL == psPath)
        return;
    free(pstShip->psObjectPath);
    pstShip->psObjectPath = psPath;
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