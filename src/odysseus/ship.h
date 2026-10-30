#ifndef __SHIP_H__
#define __SHIP_H__

#include "odysseusConfig.h"
#include "terminal.h"
#include "frame.h"
#include "socket.h"


#define SHIP_AT_ITHACA 0
#define SHIP_CONNECTED_ITHACA 1

#define LIST_VOYAGE_FIELDS 6     //Index&Total&VoyageID&Object&Destination&Reward
#define ACCEPT_VOYAGE_FIELDS 5   //OK&VoyageID&Object&Destination&Reward
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
} Ship;

void initShip(Ship *pstShip, OdysseusConfig *pstConfig);
void executeCommand(Ship *pstShip, Command *pstCommand);
void leavePort(Ship *pstShip);
void freeShip(Ship *pstShip);

#endif