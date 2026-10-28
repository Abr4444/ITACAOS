#ifndef __SERVER_H__
#define __SERVER_H__

/************************************************
 *
 * @File: server.h
 * @Purpose: Generic concurrent TCP server shared by Ithaca and Island:
 *           one thread per client, a list of clients protected by a mutex,
 *           cleanup of finished threads and closing on CTRL+C.
 *           Each server only provides what to do with a client.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/10/2026
 *
 ************************************************/

#include <errno.h>
#include <pthread.h>

#include "io.h"
#include "socket.h"

typedef struct Server Server;
typedef struct Client Client;

struct Client {
    int nFd;
    pthread_t nThread;
    int nFinished;
    int nState;       // free for each server (Ithaca: ACTIVE/LEFT/LOST, Island: OUTSIDE/QUEUED/DOCKED)
    int nTicket;      // free for each server (Island: arrival order in the queue)
    char *psName;
    Server *pstServer;
};

struct Server {
    pthread_mutex_t stMutex;    // protects the client list and the specific server's data
    int nClosing;               // set by closeServer (CTRL+C)
    int nNumClients;
    Client **ppstClients;
    void *pData;                        // the IthacaServer or IslandServer that owns it
    void (*serveClient)(Client *);      // runs in the client's thread while it is connected
    void (*endClient)(Client *);        // called with stMutex held when the connection ends
};

int initServer(Server *pstServer, void *pData, void (*serveClient)(Client *), void (*endClient)(Client *));
void runServer(Server *pstServer, int nListenFd);
void closeServer(Server *pstServer);
void printClientEvent(char *psName, char *psEvent);

#endif
