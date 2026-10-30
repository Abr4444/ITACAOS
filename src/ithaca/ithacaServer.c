/************************************************
 *
 * @File: ithacaServer.c
 * @Purpose: Ithaca's server: accepts the Odysseus connections and serves
 *           each one in its own thread, so a slow or blocked Odysseus
 *           never stops the others from being served.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/10/2026
 *
 ************************************************/

#define _GNU_SOURCE
#include "ithacaServer.h"

/************************************************
 *
 * @Name: isNameInUse
 * @Def: Checks if another connected Odysseus already uses a name.
 *       Must be called with stMutex locked.
 * @Arg: In: pstList = the connected Odysseus
 *       In: psName = name to look for
 * @Ret: 1 if the name is in use, 0 otherwise.
 *
 ************************************************/
static int isNameInUse(ClientList *pstList, char *psName) {

    Client *pstOther = NULL;
    int i = 0;

    for (i = 0; i < pstList->nNumClients; i++) {
        pstOther = pstList->ppstClients[i];
        if (0 == pstOther->nFinished && NULL != pstOther->psName && 0 == strcmp(pstOther->psName, psName)) {
            return 1;
        }
    }
    return 0;
}

/************************************************
 *
 * @Name: identifyClient
 * @Def: Handles the CONNECT ITHACA frame that Odysseus sends right after
 *       opening the connection, and stores its name. Two ships connected
 *       at the same time cannot share a name.
 * @Arg: In: pstIthaca = the server
 *       In/Out: pstClient = the client, gets psName
 * @Ret: CLIENT_ACTIVE if identified, CLIENT_LEFT if it was refused,
 *       or CLIENT_LOST if the connection broke.
 *
 ************************************************/
static int identifyClient(IthacaServer *pstIthaca, Client *pstClient) {

    ClientList *pstList = &pstIthaca->stClients;
    Frame stFrame;
    char *psReason = NULL;

    if (-1 == receiveFrame(pstClient->nFd, &stFrame)) {
        return CLIENT_LOST;
    }
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

    // Checking and storing the name under the same lock stops two ships
    // with the same name from both getting in at the same moment
    pthread_mutex_lock(&pstList->stMutex);
    if (isNameInUse(pstList, stFrame.sData)) {
        psReason = "NAME_IN_USE";
    } else {
        pstClient->psName = malloc(stFrame.nLength + 1);
        if (NULL == pstClient->psName) {
            psReason = "SERVER_OUT_OF_MEMORY";
        } else {
            strcpy(pstClient->psName, stFrame.sData);
        }
    }
    pthread_mutex_unlock(&pstList->stMutex);

    if (NULL != psReason) {
        sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_FAIL, psReason);
        return CLIENT_LEFT;
    }
    sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_OK, "OK");
    printClientEvent(pstClient->psName, "connected to Ithaca.");
    return CLIENT_ACTIVE;
}

/************************************************
 *
 * @Name: handleRequest
 * @Def: Answers one request of an identified Odysseus.
 * @Arg: In: pstClient = the client that sent it
 *       In: pstFrame = the request
 * @Ret: CLIENT_LEFT after a DISCONNECT, CLIENT_ACTIVE otherwise.
 *
 ************************************************/
static int handleRequest(Client *pstClient, Frame *pstFrame) {

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
            // Never answer a NACK with another NACK, or two processes
            // could keep bouncing them forever
            break;
        default:
            sendNack(pstClient->nFd, NACK_INVALID_TYPE);
            break;
    }
    return CLIENT_ACTIVE;
}

/************************************************
 *
 * @Name: serveOdysseus
 * @Def: Thread of one Odysseus: identifies it, then answers its requests
 *       until it disconnects, its connection breaks, or Ithaca closes.
 * @Arg: In: pArg = the Client to serve
 * @Ret: NULL.
 *
 ************************************************/
static void *serveOdysseus(void *pArg) {

    Client *pstClient = (Client *) pArg;
    IthacaServer *pstIthaca = (IthacaServer *) pstClient->pOwner;
    ClientList *pstList = &pstIthaca->stClients;
    Frame stFrame;

    pstClient->nState = identifyClient(pstIthaca, pstClient);
    while (CLIENT_ACTIVE == pstClient->nState) {
        if (-1 == receiveFrame(pstClient->nFd, &stFrame)) {
            pstClient->nState = CLIENT_LOST;
        } else {
            pstClient->nState = handleRequest(pstClient, &stFrame);
        }
    }

    pthread_mutex_lock(&pstList->stMutex);
    // When Ithaca itself is closing, the lost connections are expected
    if (CLIENT_LOST == pstClient->nState && NULL != pstClient->psName && 0 == pstList->nClosing) {
        printClientEvent(pstClient->psName, "lost the connection.");
    }
    pstClient->nFinished = 1;
    pthread_mutex_unlock(&pstList->stMutex);
    return NULL;
}

/************************************************
 *
 * @Name: initIthacaServer
 * @Def: Prepares Ithaca's server with no Odysseus connected yet.
 * @Arg: Out: pstIthaca = the server
 *       In: pstConfig = the loaded Ithaca configuration
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
int initIthacaServer(IthacaServer *pstIthaca, IthacaConfig *pstConfig) {

    pstIthaca->pstConfig = pstConfig;
    return initClientList(&pstIthaca->stClients);
}

/************************************************
 *
 * @Name: runIthacaServer
 * @Def: Accepts Odysseus connections, each one served by its own thread,
 *       until CTRL+C shuts the listening socket down.
 * @Arg: In/Out: pstIthaca = the server
 *       In: nListenFd = listening socket
 * @Ret: -
 *
 ************************************************/
void runIthacaServer(IthacaServer *pstIthaca, int nListenFd) {

    int nFd = acceptClient(nListenFd);

    while (-1 != nFd) {
        reapClients(&pstIthaca->stClients);
        if (-1 == addClient(&pstIthaca->stClients, nFd, serveOdysseus, pstIthaca)) {
            printError("Error: could not start serving a new Odysseus.\n");
            close(nFd);
        }
        nFd = acceptClient(nListenFd);
    }
    // EINVAL means CTRL+C shut the socket down, which is the normal exit.
    // Any other errno means accept broke on its own, so say why we stop
    if (EINVAL != errno) {
        printError("Error: Ithaca stopped accepting ships unexpectedly.\n");
    }
}

/************************************************
 *
 * @Name: closeIthacaServer
 * @Def: Closes every Odysseus connection and frees the server.
 * @Arg: In/Out: pstIthaca = the server
 * @Ret: -
 *
 ************************************************/
void closeIthacaServer(IthacaServer *pstIthaca) {

    closeClientList(&pstIthaca->stClients);
}
