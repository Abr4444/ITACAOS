/************************************************
 * @File: ship.c
 * @Purpose: Odysseus's state, STATUS and the dispatcher of the terminal commands.
 * @Date: 28/10/2026
 ************************************************/

#define _GNU_SOURCE
#include "ship.h"
#include "voyages.h"
#include "sailing.h"
#include "market.h"
#include "delivery.h"
#include "state.h"

/* @Name: showStatus
 * @Def: STATUS: prints gold, cargo, provisions, voyage and location.
 * @Arg: In: pstShip = the ship
 * @Ret: - */
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
    } else if (-1 != asprintf(&psMessage, "\nTotal provisions: %d kg\n\nCurrent voyage: %s -> %s\nReward: %d\nObject: %s\nCurrent location: %s\n",
                              nTotal, pstShip->psObject, pstShip->psDestination, pstShip->nReward, fileNameOf(pstShip->psObjectPath), pstShip->psLocation)) {
        printString(psMessage);
        free(psMessage);
    }
}

/* @Name: closeConnection
 * @Def: Closes the current connection, if there is one.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
void closeConnection(Ship *pstShip) {
    if (-1 != pstShip->nFd) {
        close(pstShip->nFd);
        pstShip->nFd = -1;
    }
}

/* @Name: initShip
 * @Def: Starts the ship in Ithaca with the config's gold and Aeaea in its map.
 * @Arg: Out: pstShip = the ship
 *       In: pstConfig = the configuration
 * @Ret: - */
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
    pstShip->psObjectPath = NULL;
    pstShip->nDelivered = 0;
    initMap(&pstShip->stMap);
    // voyages begins there
    if (NULL == addIsland(&pstShip->stMap, pstConfig->psAeaeaName, pstConfig->psAeaeaIp, pstConfig->nAeaeaPort))
        printString("Error: not enough memory\n");
}

/* @Name: freeVoyage
 * @Def: Frees the current voyage and forgets it. Its object stays in the
 *       ship's folder, so the next execution can go on with the voyage.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
static void freeVoyage(Ship *pstShip) {
    free(pstShip->psObjectPath);
    free(pstShip->psObject);
    free(pstShip->psDestination);
    pstShip->psObjectPath = NULL;
    pstShip->psObject = NULL;
    pstShip->psDestination = NULL;
    pstShip->nVoyageId = 0;
    pstShip->nReward = 0;
    pstShip->nDelivered = 0;
}

/* @Name: dropVoyage
 * @Def: Ends the current voyage: deletes its object from the ship's folder and forgets it.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
void dropVoyage(Ship *pstShip) {
    if (NULL != pstShip->psObjectPath)
        unlink(pstShip->psObjectPath);
    freeVoyage(pstShip);
}

/* @Name: freeShip
 * @Def: Frees the voyage, the listed voyages and the map.
 * @Arg: In/Out: pstShip = the ship
 * @Ret: - */
void freeShip(Ship *pstShip) {
    freeVoyage(pstShip);
    forgetListedVoyages(pstShip);
    freeMap(&pstShip->stMap);
}

/* @Name: executeCommand
 * @Def: Runs a parsed terminal command.
 * @Arg: In/Out: pstShip = the ship
 *       In: pstCommand = the command
 * @Ret: - */
void executeCommand(Ship *pstShip, Command *pstCommand) {
    if (SHIP_WAITING == pstShip->nState && CMD_STATUS != pstCommand->nType && CMD_MAP != pstCommand->nType) {
        printString("Waiting offshore: only STATUS and MAP can be used.\n");
        return;
    }
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
        case CMD_DELIVER:
            deliver(pstShip);
            break;
        case CMD_CLAIM:
            claim(pstShip);
            break;
        default:
            // Implemented by the next steps of the phase
            printString("Command OK\n");
            break;
    }
}

/* @Name: runLine
 * @Def: Parses a line typed in the terminal and runs it.
 * @Arg: In/Out: pstShip = the ship
 *       In/Out: psLine = the line (parseCommand modifies it)
 * @Ret: - */
void runLine(Ship *pstShip, char *psLine) {
    Command stCommand;

    switch (parseCommand(psLine, &stCommand)) {
        case PARSE_OK:
            executeCommand(pstShip, &stCommand);
            saveState(pstShip);
            break;
        case PARSE_BAD_SYNTAX:
            printString(stCommand.psUsage);
            break;
        case PARSE_UNKNOWN:
            printString("Unknown command\n");
            break;
        default:
            break;
    }
}