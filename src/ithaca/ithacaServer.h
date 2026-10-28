#ifndef __ITHACA_SERVER_H__
#define __ITHACA_SERVER_H__

#include "ithacaConfig.h"
#include "frame.h"
#include "server.h"

#define CLIENT_ACTIVE 0   // identified, keeps sending requests
#define CLIENT_LEFT 1     // said goodbye or was refused
#define CLIENT_LOST 2     // the connection broke (crash, kill, CTRL+C)

typedef struct {
    Server stServer;          // generic part: threads, client list, mutex
    IthacaConfig *pstConfig;
} IthacaServer;

int initIthacaServer(IthacaServer *pstIthaca, IthacaConfig *pstConfig);

#endif
