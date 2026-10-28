#define _GNU_SOURCE
#include "ithacaServer.h"

static void printClientEvent(char *psName, char *psEvent) {
    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, "\n>>> %s %s\n", psName, psEvent)) {
        printString(psMessage);
        free(psMessage);
    }
}

static int isNameInUse(IthacaServer *pstServer, char *psName) {
    Client *pstOther = NULL;
    int i = 0;

    for (i = 0; i < pstServer->nNumClients; i++) {
        pstOther = pstServer->ppstClients[i];
        if (0 == pstOther->nFinished && NULL != pstOther->psName && 0 == strcmp(pstOther->psName, psName))
            return 1;
    }
    return 0;
}

static int identifyClient(Client *pstClient){
    IthacaServer *pstServer = pstClient->pstServer;
    Frame stFrame;
    char *psReason = NULL;

    if (-1 == receiveFrame(pstClient->nFd, &stFrame))
        return CLIENT_LOST;
    
    psReason = checkFrame(&stFrame);
    if (NULL != psReason) {
        sendNack(pstClient->nFd, psReason);
        return CLIENT_LEFT;
    }
    if (TYPE_CONNECT_ITHACA != stFrame.nType) {
        sendFrame(pstClient->nFd, stFrame.nType, FLAGS_FAIL, "NOT_IDENTIFIED");
        return CLIENT_LEFT;
    }
    if (0 == stFrame.nLength) {
        sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_FAIL, "EMPTY_NAME");
        return CLIENT_LEFT;
    }

    pthread_mutex_lock(&pstServer->stClientsMutex);
    if (isNameInUse(pstServer, stFrame.sData)) {
        psReason = "NAME_IN_USE";
    } else {
        pstClient->psName = malloc(stFrame.nLength + 1);
        if (NULL == pstClient->psName) {
            psReason = "SERVER_OUT_OF_MEMORY";
        } else {
            strcpy(pstClient->psName, stFrame.sData);
        }
    }
    pthread_mutex_unlock(&pstServer->stClientsMutex);

    if (NULL != psReason) {
        sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_FAIL, psReason);
        return CLIENT_LEFT;
    }
    sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_OK, "OK");
    printClientEvent(pstClient->psName, "connected to Ithaca.");
    return CLIENT_ACTIVE;
}

static int handleRequest(Client *pstClient, Frame *pstFrame){
    char *psReason = NULL;

    psReason = checkFrame(pstFrame);
    if (NULL != psReason) {
        sendNack(pstClient->nFd, psReason);
        return CLIENT_ACTIVE;
    }

    switch (pstFrame->nType) {
        case TYPE_DISCONNECT_ITHACA:
            sendFrame(pstClient->nFd, TYPE_DISCONNECT_ITHACA, FLAGS_OK, "OK");
            printClientEvent(pstClient->psName, "has left Ithaca. Fair winds!");
            return CLIENT_LEFT;
        case TYPE_CONNECT_ITHACA:
            sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_FAIL, "ALREADY_CONNECTED");
            break;
        case TYPE_LIST_VOYAGES:
        case TYPE_ACCEPT:
        case TYPE_CLAIM:
            // Filled in by the next steps of the phase
            sendFrame(pstClient->nFd, pstFrame->nType, FLAGS_FAIL, "NOT_IMPLEMENTED");
            break;
        case TYPE_NACK:
            break;
        default:
            sendNack(pstClient->nFd, NACK_INVALID_TYPE);
            break;
    }
    return CLIENT_ACTIVE;
}

static void* serveClient(void * pArg){
    Client* pstClient = (Client*) pArg;
    IthacaServer *pstServer = pstClient->pstServer;
    Frame stFrame;
    int nState = CLIENT_ACTIVE;
    
    nState = identifyClient(pstClient);
    while (CLIENT_ACTIVE == nState){
        if (-1 == receiveFrame(pstClient->nFd, &stFrame))
            nState = CLIENT_LOST;
        else 
            nState = handleRequest(pstClient, &stFrame);
    }

    pthread_mutex_lock(&pstServer->stClientsMutex);
    if (CLIENT_LOST == nState && NULL != pstClient->psName && 0 == pstServer->nClosing)
        printClientEvent(pstClient->psName, "lost the connection.");

    pstClient->nFinished = 1;
    pthread_mutex_unlock(&pstServer->stClientsMutex);
    return NULL;
}

static void freeClient(Client *pstClient){
    close(pstClient->nFd);
    free(pstClient->psName);
    free(pstClient);
}

static void reapClients(IthacaServer *pstServer){
    Client *pstClient = NULL;
    int i = 0;

    pthread_mutex_lock(&pstServer->stClientsMutex);
    while(i < pstServer->nNumClients){
        pstClient = pstServer->ppstClients[i];
        if(1 == pstClient->nFinished){
            pthread_join(pstClient->nThread, NULL);
            freeClient(pstClient);
            pstServer->nNumClients--;
            pstServer->ppstClients[i] = pstServer->ppstClients[pstServer->nNumClients];
        } else 
            i++;
    }
    pthread_mutex_unlock(&pstServer->stClientsMutex);
}

static int addClient(IthacaServer *pstServer, int nFd){
    Client *pstClient = NULL;
    Client **ppstMore = NULL;
    int nResult = 0;

    pstClient = malloc(sizeof(Client));
    if(NULL == pstClient)
        return -1;
    
    pstClient->nFd = nFd;
    pstClient->nFinished = 0;
    pstClient->psName = NULL;
    pstClient->pstServer = pstServer;

    pthread_mutex_lock(&pstServer->stClientsMutex);
    ppstMore = realloc(pstServer->ppstClients, (pstServer->nNumClients + 1) * sizeof(Client *));
    if (NULL == ppstMore)
        nResult = -1;
    else {
        pstServer->ppstClients = ppstMore;
        if (0 != pthread_create(&pstClient->nThread, NULL, serveClient, pstClient)) {
            nResult = -1;
        } else {
            pstServer->ppstClients[pstServer->nNumClients] = pstClient;
            pstServer->nNumClients++;
        }
    }
    pthread_mutex_unlock(&pstServer->stClientsMutex);

    if (-1 == nResult) {
        free(pstClient);
    }
    return nResult;
}

static int acceptClient(int nServerFd){
    int nFd = accept(nServerFd, NULL, NULL);
    while(-1 == nFd && ECONNABORTED == errno){ //dropped between connect and accept
        nFd = accept(nServerFd, NULL, NULL);
    }
    return nFd;
}

int initIthacaServer(IthacaServer *pstServer, IthacaConfig *pstConfig) {
    pstServer->pstConfig = pstConfig;
    pstServer->nNumClients = 0;
    pstServer->nClosing = 0;
    pstServer->ppstClients = NULL;

    if (0 != pthread_mutex_init(&pstServer->stClientsMutex, NULL)){
        printError("Error: could not create client mutex\n");
        return -1;
    }
    return 0;
}

void runIthacaServer(IthacaServer *pstServer, int nServerFd){
    int nClientFd = acceptClient(nServerFd);

    while (-1 != nClientFd) {
        reapClients(pstServer);
        if (-1 == addClient(pstServer, nClientFd)){
            printError("Error: could not add an Odysseus\n");
            close(nClientFd);
        }
        nClientFd = acceptClient(nServerFd);
    }
    if (EINVAL != errno) { //if not CNTRL C sum is wrong
        printError("Error: could not accept more connections.\n");
    }
}

void closeIthacaServer(IthacaServer *pstServer){
    int i = 0;
    
    pthread_mutex_lock(&pstServer->stClientsMutex);
    pstServer->nClosing = 1;
    for (i = 0; i < pstServer->nNumClients; i++) {
        shutdown(pstServer->ppstClients[i]->nFd, SHUT_RDWR);
    }
    pthread_mutex_unlock(&pstServer->stClientsMutex);

    for (i = 0; i < pstServer->nNumClients; i++) {
        pthread_join(pstServer->ppstClients[i]->nThread, NULL);
        freeClient(pstServer->ppstClients[i]);
    }
    free(pstServer->ppstClients);
    pthread_mutex_destroy(&pstServer->stClientsMutex);
}