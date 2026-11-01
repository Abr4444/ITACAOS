#ifndef __SERVER_H__
#define __SERVER_H__

/************************************************
 * @File: server.h
 * @Purpose: Pieces shared by the Ithaca and Island servers: the list of
 *           connected clients (one thread each) protected by a mutex,
 *           accepting connections, cleaning up finished threads and
 *           closing every connection on CTRL+C. What each server does
 *           with a client stays in its own thread function.
 * @Date: 30/10/2026
 ************************************************/

#include <errno.h>
#include <pthread.h>

#include "io.h"
#include "socket.h"

// One connected Odysseus, served by its own thread
typedef struct {
    int nFd;
    pthread_t nThread;
    int nFinished;      // raised by the thread as its very last step
    int nState;         // each server's own states (Ithaca: CLIENT_, Island: SHIP_)
    int nTicket;        // Island: arrival order in the FIFO queue
    char *psName;
    void *pOwner;       // the IthacaServer or IslandServer serving it
} Client;

// Every connected client. stMutex also protects the owner's shared data.
typedef struct {
    pthread_mutex_t stMutex;
    int nClosing;       // raised by closeClientList (CTRL+C)
    int nNumClients;
    Client **ppstClients;
} ClientList;

int initClientList(ClientList *pstList);
int acceptClient(int nListenFd);
void reapClients(ClientList *pstList);
int addClient(ClientList *pstList, int nFd, void *(*pThread)(void *), void *pOwner);
void closeClientList(ClientList *pstList);
void printClientEvent(char *psName, char *psEvent);

#endif
