#define _GNU_SOURCE
#include "server.h"

void printClientEvent(char *psName, char *psEvent) {
    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, "\n>>> %s %s\n", psName, psEvent)) {
        printString(psMessage);
        free(psMessage);
    }
}

// thread of one client: the specific server serves it, then cleans up after it
static void *clientThread(void *pArg) {
    Client *pstClient = (Client *) pArg;
    Server *pstServer = pstClient->pstServer;

    pstServer->serveClient(pstClient);

    pthread_mutex_lock(&pstServer->stMutex);
    if (NULL != pstServer->endClient)
        pstServer->endClient(pstClient);
    pstClient->nFinished = 1;
    pthread_mutex_unlock(&pstServer->stMutex);
    return NULL;
}

static void freeClient(Client *pstClient) {
    close(pstClient->nFd);
    free(pstClient->psName);
    free(pstClient);
}

// joins and frees the threads that already finished
static void reapClients(Server *pstServer) {
    int i = 0;

    pthread_mutex_lock(&pstServer->stMutex);
    while (i < pstServer->nNumClients) {
        if (1 == pstServer->ppstClients[i]->nFinished) {
            pthread_join(pstServer->ppstClients[i]->nThread, NULL);
            freeClient(pstServer->ppstClients[i]);
            pstServer->nNumClients--;
            pstServer->ppstClients[i] = pstServer->ppstClients[pstServer->nNumClients];
        } else
            i++;
    }
    pthread_mutex_unlock(&pstServer->stMutex);
}

static int addClient(Server *pstServer, int nFd) {
    Client *pstClient = calloc(1, sizeof(Client));
    Client **ppstMore = NULL;
    int nResult = -1;

    if (NULL == pstClient)
        return -1;
    pstClient->nFd = nFd;
    pstClient->pstServer = pstServer;

    pthread_mutex_lock(&pstServer->stMutex);
    ppstMore = realloc(pstServer->ppstClients, (pstServer->nNumClients + 1) * sizeof(Client *));
    if (NULL != ppstMore) {
        pstServer->ppstClients = ppstMore;
        if (0 == pthread_create(&pstClient->nThread, NULL, clientThread, pstClient)) {
            pstServer->ppstClients[pstServer->nNumClients++] = pstClient;
            nResult = 0;
        }
    }
    pthread_mutex_unlock(&pstServer->stMutex);

    if (-1 == nResult)
        free(pstClient);
    return nResult;
}

int initServer(Server *pstServer, void *pData, void (*serveClient)(Client *), void (*endClient)(Client *)) {
    memset(pstServer, 0, sizeof(Server));
    pstServer->pData = pData;
    pstServer->serveClient = serveClient;
    pstServer->endClient = endClient;
    if (0 != pthread_mutex_init(&pstServer->stMutex, NULL)) {
        printError("Error: could not create the server mutex.\n");
        return -1;
    }
    return 0;
}

// accepts clients until the listening socket is shut down (CTRL+C)
void runServer(Server *pstServer, int nListenFd) {
    int nFd = 0;

    while (1) {
        nFd = accept(nListenFd, NULL, NULL);
        if (-1 == nFd && (ECONNABORTED == errno || EINTR == errno))
            continue;   // dropped connection / signal: after CTRL+C the next accept gives EINVAL
        if (-1 == nFd)
            break;
        reapClients(pstServer);
        if (-1 == addClient(pstServer, nFd)) {
            printError("Error: could not add a client.\n");
            close(nFd);
        }
    }
    if (EINVAL != errno)    // EINVAL is the normal result of CTRL+C
        printError("Error: could not accept more connections.\n");
}

// closes every connection, waits for all the threads and frees everything
void closeServer(Server *pstServer) {
    int i = 0;

    pthread_mutex_lock(&pstServer->stMutex);
    pstServer->nClosing = 1;
    for (i = 0; i < pstServer->nNumClients; i++)
        shutdown(pstServer->ppstClients[i]->nFd, SHUT_RDWR);
    pthread_mutex_unlock(&pstServer->stMutex);

    for (i = 0; i < pstServer->nNumClients; i++) {
        pthread_join(pstServer->ppstClients[i]->nThread, NULL);
        freeClient(pstServer->ppstClients[i]);
    }
    free(pstServer->ppstClients);
    pthread_mutex_destroy(&pstServer->stMutex);
}
