#ifndef __SHIP_H__
#define __SHIP_H__

/************************************************
 * @File: ship.h
 * @Purpose: Odysseus's state, STATUS and the dispatcher of the terminal commands.
 * @Date: 28/10/2026
 ************************************************/

#include "odysseusConfig.h"
#include "terminal.h"
#include "frame.h"
#include "socket.h"
#include "map.h"
#include "transfer.h"

#define ITHACA_NAME "Ithaca"
#define PROMPT "$ "

#define SHIP_AT_ITHACA 0 // in Ithaca's harbor, not connected
#define SHIP_CONNECTED_ITHACA 1
#define SHIP_DOCKED 2 // docked at psLocation, connected to it
#define SHIP_AT_SEA 3 // left psLocation but could not dock anywhere
#define SHIP_LOST 4 // ran out of provisions: the process ends
#define SHIP_WAITING 5 //offshore, waiting for berth: only STATUS and MAP work

typedef struct {
    OdysseusConfig *pstConfig;  
    int nState;
    int nFd;// socket of the current connection, -1 if none
    char *psLocation;           // island where the ship is
    int nGold;
    int nVoyageId;              
    char *psObject;             
    char *psDestination;
    int nReward;
    int *pnListedIds;// voyage IDs of the last LIST VOYAGES
    int nNumListed;
    Map stMap;  // the islands the ship knows, Aeaea first
    char *psObjectPath;
    int nDelivered;
    int *pnStop;     // the CTRL+C flag of main
} Ship;

void initShip(Ship *pstShip, OdysseusConfig *pstConfig);
void executeCommand(Ship *pstShip, Command *pstCommand);
void runLine(Ship *pstShip, char *psLine);
void closeConnection(Ship *pstShip);
void freeShip(Ship *pstShip);
void dropVoyage(Ship *pstShip);

#endif