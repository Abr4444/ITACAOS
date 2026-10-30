#ifndef __ITHACA_SERVER_H__
#define __ITHACA_SERVER_H__

/************************************************
 *
 * @File: ithacaServer.h
 * @Purpose: Ithaca's server: accepts the Odysseus connections and serves
 *           each one in its own thread, so a slow or blocked Odysseus
 *           never stops the others from being served.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/10/2026
 *
 ************************************************/

#include "ithacaConfig.h"
#include "frame.h"
#include "server.h"

// How the thread of an Odysseus ends up (Client nState)
#define CLIENT_ACTIVE 0   // identified, keeps sending requests
#define CLIENT_LEFT 1     // said goodbye or was refused
#define CLIENT_LOST 2     // the connection broke (crash, kill, CTRL+C)

typedef struct {
    ClientList stClients;       // connected Odysseus, one thread each
    IthacaConfig *pstConfig;
} IthacaServer;

int initIthacaServer(IthacaServer *pstIthaca, IthacaConfig *pstConfig);
void runIthacaServer(IthacaServer *pstIthaca, int nListenFd);
void closeIthacaServer(IthacaServer *pstIthaca);

#endif
