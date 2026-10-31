#ifndef __SHIP_H__
#define __SHIP_H__

#include "odysseusConfig.h"
#include "terminal.h"
#include "frame.h"
#include "socket.h"
#include "map.h"

#define ITHACA_NAME "Ithaca"

#define SHIP_AT_ITHACA 0 // in Ithaca's harbor, not connected
#define SHIP_CONNECTED_ITHACA 1
#define SHIP_DOCKED 2 // docked at psLocation, connected to it
#define SHIP_AT_SEA 3 // left psLocation but could not dock anywhere
#define SHIP_LOST 4 // ran out of provisions: the process ends

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
} Ship;

void initShip(Ship *pstShip, OdysseusConfig *pstConfig);
void executeCommand(Ship *pstShip, Command *pstCommand);
void closeConnection(Ship *pstShip);
void freeShip(Ship *pstShip);

#endif