#define _GNU_SOURCE
#include "ithacaServer.h"

// stMutex held
static int isNameInUse(Server *pstServer, char *psName) {
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
    Server *pstServer = pstClient->pstServer;
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

    pthread_mutex_lock(&pstServer->stMutex);
    if (isNameInUse(pstServer, stFrame.sData)) {
        psReason = "NAME_IN_USE";
    } else {
        pstClient->psName = strdup(stFrame.sData);
        if (NULL == pstClient->psName)
            psReason = "SERVER_OUT_OF_MEMORY";
    }
    pthread_mutex_unlock(&pstServer->stMutex);

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

// runs in the client's thread (called by server.c)
static void serveOdysseus(Client *pstClient) {
    Frame stFrame;

    pstClient->nState = identifyClient(pstClient);
    while (CLIENT_ACTIVE == pstClient->nState) {
        if (-1 == receiveFrame(pstClient->nFd, &stFrame))
            pstClient->nState = CLIENT_LOST;
        else
            pstClient->nState = handleRequest(pstClient, &stFrame);
    }
}

// stMutex held (called by server.c when the connection ends)
static void endOdysseus(Client *pstClient) {
    if (CLIENT_LOST == pstClient->nState && NULL != pstClient->psName && 0 == pstClient->pstServer->nClosing)
        printClientEvent(pstClient->psName, "lost the connection.");
}

int initIthacaServer(IthacaServer *pstIthaca, IthacaConfig *pstConfig) {
    pstIthaca->pstConfig = pstConfig;
    return initServer(&pstIthaca->stServer, pstIthaca, serveOdysseus, endOdysseus);
}
