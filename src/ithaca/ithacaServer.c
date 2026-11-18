/************************************************
 * @File: ithacaServer.c
 * @Purpose: Ithaca's server: accepts the Odysseus connections and serves
 *           each one in its own thread, so a slow or blocked Odysseus
 *           never stops the others from being served.
 * @Date: 30/10/2026
 ************************************************/

#define _GNU_SOURCE
#include "ithacaServer.h"

/************************************************
 * @Name: isNameInUse
 * @Def: Checks if another connected Odysseus already uses a name.
 *       Must be called with stMutex locked.
 * @Arg: In: pstList = the connected Odysseus
 *       In: psName = name to look for
 * @Ret: 1 if the name is in use, 0 otherwise.
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
 * @Name: identifyClient
 * @Def: Handles the CONNECT ITHACA frame that Odysseus sends right after
 *       opening the connection, and stores its name. Two ships connected
 *       at the same time cannot share a name.
 * @Arg: In: pstIthaca = the server
 *       In/Out: pstClient = the client, gets psName
 * @Ret: CLIENT_ACTIVE if identified, CLIENT_LEFT if it was refused,
 *       or CLIENT_LOST if the connection broke.
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
    printString("\n");
    sendFrame(pstClient->nFd, TYPE_CONNECT_ITHACA, FLAGS_OK, "OK");
    printClientEvent(pstClient->psName, "connected to Ithaca.");
    return CLIENT_ACTIVE;
}

/************************************************
 * @Name: listVoyages
 * @Def: LIST VOYAGES: sends one frame per available voyage, or "0" if
 *       none is left. An Odysseus that still holds a voyage and asks for
 *       the list again has restarted, so that voyage has failed.
 * @Arg: In/Out: pstIthaca = the server
 *       In: pstClient = the Odysseus that asked
 * @Ret: -
 ************************************************/
static void listVoyages(IthacaServer *pstIthaca, Client *pstClient) {

    IthacaConfig *pstConfig = pstIthaca->pstConfig;
    Voyage *pstVoyage = NULL;
    char *psData = NULL;
    int nTotal = 0;
    int nIndex = 0;
    int nError = 0;

    pthread_mutex_lock(&pstIthaca->stVoyagesMutex);
    for (int i = 0; i < pstConfig->nNumVoyages; i++) {
        pstVoyage = &pstConfig->pstVoyages[i];
        if (VOYAGE_ASSIGNED == pstVoyage->nState && 0 == strcmp(pstVoyage->psHolder, pstClient->psName))
            pstVoyage->nState = VOYAGE_FAILED;
        if (VOYAGE_AVAILABLE == pstVoyage->nState)
            nTotal++;
    }
    if (0 == nTotal)
        sendFrame(pstClient->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, "0");

    for (int i = 0; i < pstConfig->nNumVoyages && 0 == nError; i++) {
        pstVoyage = &pstConfig->pstVoyages[i];
        if (VOYAGE_AVAILABLE == pstVoyage->nState) {
            nIndex++;
            if (-1 == asprintf(&psData, "%d&%d&%d&%s&%s&%d", nIndex, nTotal, pstVoyage->nId,
                               pstVoyage->psObject, pstVoyage->psDestination, pstVoyage->nReward)) {
                // Odysseus stops reading the list when it gets an error
                sendFrame(pstClient->nFd, TYPE_LIST_VOYAGES, FLAGS_FAIL, "SERVER_OUT_OF_MEMORY");
                nError = 1;
            } else {
                sendFrame(pstClient->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, psData);
                free(psData);
            }
        }
    }
    pthread_mutex_unlock(&pstIthaca->stVoyagesMutex);

    printClientEvent(pstClient->psName, "requested available voyages.");
}

/************************************************
 * @Name: toVoyageId
 * @Def: Converts the <VoyageID> of an ACCEPT frame.
 * @Arg: In: psText = the DATA of the frame
 * @Ret: The ID, or -1 if it is not a positive number of up to 9 digits.
 ************************************************/
static int toVoyageId(char *psText) {

    int nId = 0;
    int i = 0;

    for (i = 0; '\0' != psText[i]; i++) {
        if (9 <= i || '0' > psText[i] || '9' < psText[i])
            return -1;
        nId = nId * 10 + (psText[i] - '0');
    }
    if (0 == nId)
        return -1;
    return nId;
}

/* @Name: objectExists
 * @Def: Checks that a voyage's image can be opened for reading.
 * @Arg: In: psPath = path of the image, or NULL
 * @Ret: 1 if it exists, 0 otherwise. */
static int objectExists(char *psPath) {
    int nFd = -1;

    if (NULL != psPath)
        nFd = open(psPath, O_RDONLY);
    if (-1 == nFd)
        return 0;
    close(nFd);
    return 1;
}

/************************************************
 * @Name: acceptVoyage
 * @Def: assigns a voyage to the Odysseus if it is still available.
 * @Arg: In/Out: pstIthaca = the server
 *       In: pstClient = the Odysseus that accepts it
 *       In: pstFrame = the request, with the voyage ID
 * @Ret: The voyage assigned, or NULL if it was refused.
 ************************************************/
static Voyage* acceptVoyage(IthacaServer *pstIthaca, Client *pstClient, Frame *pstFrame) {

    IthacaConfig *pstConfig = pstIthaca->pstConfig;
    Voyage *pstVoyage = NULL;
    char *psReason = NULL;
    char *psAnswer = NULL;
    char *psMessage = NULL;
    int nId = toVoyageId(pstFrame->sData);

    if (-1 == nId) {
        sendFrame(pstClient->nFd, TYPE_ACCEPT, FLAGS_FAIL, "INVALID_VOYAGE_ID");
        return NULL;
    }
    pthread_mutex_lock(&pstIthaca->stVoyagesMutex);
    for (int i = 0; i < pstConfig->nNumVoyages && NULL == psReason; i++) {
            if (VOYAGE_ASSIGNED == pstConfig->pstVoyages[i].nState && 
                0 == strcmp(pstConfig->pstVoyages[i].psHolder, pstClient->psName))
            psReason = "ALREADY_HAS_VOYAGE";
    }
    if (NULL == psReason) {
        pstVoyage = &pstConfig->pstVoyages[nId - 1];
        if (nId > pstConfig->nNumVoyages || VOYAGE_AVAILABLE != pstVoyage->nState)
            psReason = "VOYAGE_NOT_AVAILABLE";
        else if (0 == objectExists(pstVoyage->psPath))
            psReason = "OBJECT_NOT_FOUND";
        else {
            pstVoyage->psHolder = malloc(strlen(pstClient->psName) + 1);
            if (NULL == pstVoyage->psHolder ||
                -1 == asprintf(&psAnswer, "OK&%d&%s&%s&%d", pstVoyage->nId, pstVoyage->psObject,
                            pstVoyage->psDestination, pstVoyage->nReward)) {
                free(pstVoyage->psHolder);
                pstVoyage->psHolder = NULL;
                psReason = "OUT_OF_MEMORY";
            } else {
                strcpy(pstVoyage->psHolder, pstClient->psName);
                pstVoyage->nState = VOYAGE_ASSIGNED;
            }
        }
    }
    pthread_mutex_unlock(&pstIthaca->stVoyagesMutex);

    if (NULL != psReason) {
        sendFrame(pstClient->nFd, TYPE_ACCEPT, FLAGS_FAIL, psReason);
        return NULL;
    }
    sendFrame(pstClient->nFd, TYPE_ACCEPT, FLAGS_OK, psAnswer);
    free(psAnswer);
    if (-1 != asprintf(&psMessage, "\n>>> Voyage %d assigned to %s.\n", nId, pstClient->psName)) {
        printString(psMessage);
        free(psMessage);
    }
    return pstVoyage;
}

/* @Name: sendObject
 * @Def: Sends the image of a just-assigned voyage to its Odysseus
 * @Arg: In/Out: pstIthaca = the server (for the voyages mutex)
 *       In: pstClient = the Odysseus that accepted it
 *       In/Out: pstVoyage = the voyage assigned to it
 * @Ret: - */
static void sendObject(IthacaServer *pstIthaca, Client *pstClient, Voyage *pstVoyage) {
    char *psMessage = NULL;

    if (-1 != asprintf(&psMessage, ">>> Sending %s to %s...\n", pstVoyage->psObject, pstClient->psName)) {
        printString(psMessage);
        free(psMessage);
    }
    if (0 == sendFile(pstClient->nFd, "OBJECT", pstVoyage->psPath, fileNameOf(pstVoyage->psPath), FLAGS_OK)) {
        printString("\n>>> Object transferred successfully.\n");
        return;
    }

    // the ship didn't get the image
    pthread_mutex_lock(&pstIthaca->stVoyagesMutex);
    pstVoyage->nState = VOYAGE_AVAILABLE;
    free(pstVoyage->psHolder);
    pstVoyage->psHolder = NULL;
    pthread_mutex_unlock(&pstIthaca->stVoyagesMutex);
    if (-1 != asprintf(&psMessage, "\n>>> Could not send %s to %s. Voyage %d is available again.\n", pstVoyage->psObject, pstClient->psName, pstVoyage->nId)) {
        printString(psMessage);
        free(psMessage);
    }
}

/************************************************
 * @Name: handleRequest
 * @Def: Answers one request of an identified Odysseus.
 * @Arg: In: pstClient = the client that sent it
 *       In: pstFrame = the request
 * @Ret: CLIENT_LEFT after a DISCONNECT, CLIENT_ACTIVE otherwise.
 ************************************************/
static int handleRequest(IthacaServer *pstIthaca, Client *pstClient, Frame *pstFrame) {

    char *psReason = NULL;
    Voyage *pstVoyage = NULL;

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
            listVoyages(pstIthaca, pstClient);
            break;
        case TYPE_ACCEPT:
            pstVoyage = acceptVoyage(pstIthaca, pstClient, pstFrame);
            if (NULL != pstVoyage)
                sendObject(pstIthaca, pstClient, pstVoyage);
            break;
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
 * @Name: serveOdysseus
 * @Def: Thread of one Odysseus: identifies it, then answers its requests
 *       until it disconnects, its connection breaks, or Ithaca closes.
 * @Arg: In: pArg = the Client to serve
 * @Ret: NULL.
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
            pstClient->nState = handleRequest(pstIthaca, pstClient, &stFrame);
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
 * @Name: initIthacaServer
 * @Def: Prepares Ithaca's server with no Odysseus connected yet.
 * @Arg: Out: pstIthaca = the server
 *       In: pstConfig = the loaded Ithaca configuration
 * @Ret: 0 on success, -1 on error.
 ************************************************/
int initIthacaServer(IthacaServer *pstIthaca, IthacaConfig *pstConfig) {

    pstIthaca->pstConfig = pstConfig;
    if (0 != pthread_mutex_init(&pstIthaca->stVoyagesMutex, NULL)) {
        printError("Error: could not create the voyages mutex.\n");
        return -1;
    }
    if (-1 == initClientList(&pstIthaca->stClients)) {
        pthread_mutex_destroy(&pstIthaca->stVoyagesMutex);
        return -1;
    }
    return 0;
}

/************************************************
 * @Name: runIthacaServer
 * @Def: Accepts Odysseus connections, each one served by its own thread,
 *       until CTRL+C shuts the listening socket down.
 * @Arg: In/Out: pstIthaca = the server
 *       In: nListenFd = listening socket
 * @Ret: -
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
 * @Name: closeIthacaServer
 * @Def: Closes every Odysseus connection and frees the server.
 * @Arg: In/Out: pstIthaca = the server
 * @Ret: -
 ************************************************/
void closeIthacaServer(IthacaServer *pstIthaca) {

    closeClientList(&pstIthaca->stClients);
    pthread_mutex_destroy(&pstIthaca->stVoyagesMutex);
}

