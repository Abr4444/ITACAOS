/************************************************
 *
 * @File: server.c
 * @Purpose: Pieces shared by the Ithaca and Island servers: the list of
 *           connected clients (one thread each) protected by a mutex,
 *           accepting connections, cleaning up finished threads and
 *           closing every connection on CTRL+C. What each server does
 *           with a client stays in its own thread function.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "server.h"

/************************************************
 *
 * @Name: printClientEvent
 * @Def: Prints a ">>> <name> <event>" line on the server's screen.
 * @Arg: In: psName = name of the Odysseus
 *       In: psEvent = what happened, e.g. "connected to Ithaca."
 * @Ret: -
 *
 ************************************************/
void printClientEvent(char *psName, char *psEvent) {

    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, "\n>>> %s %s\n", psName, psEvent)) {
        printString(psMessage);
        free(psMessage);
    }
}

/************************************************
 *
 * @Name: freeClient
 * @Def: Closes the socket of a client whose thread has been joined
 *       and frees it.
 * @Arg: In: pstClient = client to free
 * @Ret: -
 *
 ************************************************/
static void freeClient(Client *pstClient) {

    close(pstClient->nFd);
    free(pstClient->psName);
    free(pstClient);
}

/************************************************
 *
 * @Name: initClientList
 * @Def: Prepares an empty client list and its mutex.
 * @Arg: Out: pstList = the list
 * @Ret: 0 on success, -1 if the mutex could not be created.
 *
 ************************************************/
int initClientList(ClientList *pstList) {

    pstList->nClosing = 0;
    pstList->nNumClients = 0;
    pstList->ppstClients = NULL;
    if (0 != pthread_mutex_init(&pstList->stMutex, NULL)) {
        printError("Error: could not create the client list mutex.\n");
        return -1;
    }
    return 0;
}

/************************************************
 *
 * @Name: acceptClient
 * @Def: Waits for the next connection. A client that hangs up before
 *       being accepted (ECONNABORTED) is skipped.
 * @Arg: In: nListenFd = listening socket
 * @Ret: The new socket, or -1 when accept fails. errno is EINVAL when
 *       CTRL+C shut the listening socket down.
 *
 ************************************************/
int acceptClient(int nListenFd) {

    int nFd = accept(nListenFd, NULL, NULL);

    while (-1 == nFd && (ECONNABORTED == errno || EINTR == errno)) {
        nFd = accept(nListenFd, NULL, NULL);
    }
    return nFd;
}

/************************************************
 *
 * @Name: reapClients
 * @Def: Joins and frees the threads that have finished, so a long-running
 *       server does not keep the memory of every ship that ever came.
 * @Arg: In/Out: pstList = the client list
 * @Ret: -
 *
 ************************************************/
void reapClients(ClientList *pstList) {

    Client *pstClient = NULL;
    int i = 0;

    pthread_mutex_lock(&pstList->stMutex);
    while (i < pstList->nNumClients) {
        pstClient = pstList->ppstClients[i];
        if (1 == pstClient->nFinished) {
            // Its last step was raising nFinished, so this join is immediate
            pthread_join(pstClient->nThread, NULL);
            freeClient(pstClient);
            pstList->nNumClients--;
            pstList->ppstClients[i] = pstList->ppstClients[pstList->nNumClients];
        } else {
            i++;
        }
    }
    pthread_mutex_unlock(&pstList->stMutex);
}

/************************************************
 *
 * @Name: addClient
 * @Def: Adds a new connection to the list and starts its thread.
 * @Arg: In/Out: pstList = the client list
 *       In: nFd = socket of the new connection
 *       In: pThread = function the thread runs, e.g. serveShip. It gets
 *           the new Client as its argument.
 *       In: pOwner = the server the client belongs to, stored in pOwner
 * @Ret: 0 on success, -1 on error (the caller closes nFd).
 *
 ************************************************/
int addClient(ClientList *pstList, int nFd, void *(*pThread)(void *), void *pOwner) {

    Client *pstClient = NULL;
    Client **ppstMore = NULL;
    int nResult = -1;

    pstClient = malloc(sizeof(Client));
    if (NULL == pstClient) {
        return -1;
    }
    pstClient->nFd = nFd;
    pstClient->nFinished = 0;
    pstClient->nState = 0;
    pstClient->nTicket = 0;
    pstClient->psName = NULL;
    pstClient->pOwner = pOwner;

    pthread_mutex_lock(&pstList->stMutex);
    ppstMore = realloc(pstList->ppstClients, (pstList->nNumClients + 1) * sizeof(Client *));
    if (NULL != ppstMore) {
        pstList->ppstClients = ppstMore;
        if (0 == pthread_create(&pstClient->nThread, NULL, pThread, pstClient)) {
            pstList->ppstClients[pstList->nNumClients] = pstClient;
            pstList->nNumClients++;
            nResult = 0;
        }
    }
    pthread_mutex_unlock(&pstList->stMutex);

    if (-1 == nResult) {
        free(pstClient);
    }
    return nResult;
}

/************************************************
 *
 * @Name: closeClientList
 * @Def: Closes every connection, waits for all the threads and frees the
 *       list. Shutting a socket down makes its thread's read() fail, so
 *       every thread ends on its own. The joins happen without the lock,
 *       because each ending thread still needs it for its last step.
 * @Arg: In/Out: pstList = the client list
 * @Ret: -
 *
 ************************************************/
void closeClientList(ClientList *pstList) {

    int i = 0;

    pthread_mutex_lock(&pstList->stMutex);
    pstList->nClosing = 1;
    for (i = 0; i < pstList->nNumClients; i++) {
        shutdown(pstList->ppstClients[i]->nFd, SHUT_RDWR);
    }
    pthread_mutex_unlock(&pstList->stMutex);

    for (i = 0; i < pstList->nNumClients; i++) {
        pthread_join(pstList->ppstClients[i]->nThread, NULL);
    }
    // Free only after every thread has ended: a thread that is still
    // finishing may look at the other clients (e.g. the name check)
    for (i = 0; i < pstList->nNumClients; i++) {
        freeClient(pstList->ppstClients[i]);
    }
    free(pstList->ppstClients);
    pthread_mutex_destroy(&pstList->stMutex);
}
