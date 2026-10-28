#ifndef __SHIP_H__
#define __SHIP_H__

#include "odysseusConfig.h"
#include "terminal.h"
#include "frame.h"
#include "socket.h"


#define SHIP_AT_ITHACA 0
#define SHIP_CONNECTED_ITHACA 1

typedef struct {
    OdysseusConfig *pstConfig;
    int nState;
    int nFd;// socket of the current connection, -1 if none
} Ship;

void initShip(Ship *pstShip, OdysseusConfig *pstConfig);
void executeCommand(Ship *pstShip, Command *pstCommand);
void leavePort(Ship *pstShip);

#endif