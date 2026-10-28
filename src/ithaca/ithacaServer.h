#ifndef __ITHACA_SERVER_H__
#define __ITHACA_SERVER_H__

#include <errno.h>
#include <pthread.h>

#include "ithacaConfig.h"
#include "frame.h"
#include "socket.h"

#define CLIENT_ACTIVE 0   // identified, keeps sending requests
#define CLIENT_LEFT 1     // said goodbye or was refused
#define CLIENT_LOST 2     // the connection broke (crash, kill, CTRL+C)

typedef struct Client Client;

typedef struct{
    IthacaConfig *pstConfig;
    pthread_mutex_t stClientsMutex;
    int nClosing; //server closing
    int nNumClients;
    Client **ppstClients; //pointer to a list of pointers to Clients
} IthacaServer;


//for each Odysseus a thread that manages it so that is not blocking
struct Client{
    int nFd;
    pthread_t nThread;
    int nFinished;
    char *psName;
    IthacaServer *pstServer; //lets thread reach shared server data
};

int initIthacaServer(IthacaServer *pstServer, IthacaConfig *pstConfig);
void runIthacaServer(IthacaServer *pstServer, int nServerFd);
void closeIthacaServer(IthacaServer *pstServer);

#endif