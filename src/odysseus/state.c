/************************************************
 * @File: state.c
 * @Purpose: Saves the ship in its folder after every command and
 *           recovers it when Odysseus starts again.
 * @Date: 21/11/2026
 ************************************************/
#define _GNU_SOURCE
#include "state.h"

/************************************************
 * @Name: openState
 * @Def: Opens STATE_FILE inside the ship's folder.
 * @Arg: In: pstConfig = the configuration with the folder
 *       In: nFlags = flags for open()
 * @Ret: The file descriptor, or -1 on error.
 ************************************************/
static int openState(OdysseusConfig *pstConfig, int nFlags) {
    char *psPath = NULL;
    int nFd = -1;

    if (-1 != asprintf(&psPath, "%s/%s", pstConfig->psFolder, STATE_FILE)) {
        nFd = open(psPath, nFlags, 0644);
        free(psPath);
    }
    return nFd;
}
/************************************************
 * @Name: writeVoyage
 * @Def: Writes the active voyage in one line, or 0 if there is none.
 * @Arg: In: nFd = the state file
 *       In: pstShip = the ship
 * @Ret: -
 ************************************************/
static void writeVoyage(int nFd, Ship *pstShip) {
    char *psText = NULL;

    if (0 == pstShip->nVoyageId) {
        writeText(nFd, "0\n");
        return;
    }
    if (-1 != asprintf(&psText, "%d %s %s %d %s %d\n", pstShip->nVoyageId, pstShip->psObject, pstShip->psDestination,
                       pstShip->nReward, fileNameOf(pstShip->psObjectPath), pstShip->nDelivered)) {
        writeText(nFd, psText);
        free(psText);
    }
}
/************************************************
 * @Name: readVoyage
 * @Def: Reads the voyage line written by writeVoyage. Its object is
 *       already in the ship's folder.
 * @Arg: In: nFd = the state file
 *       In/Out: pstShip = the ship, without a voyage yet
 * @Ret: 0 on success, -1 if memory runs out.
 ************************************************/
static int readVoyage(int nFd, Ship *pstShip) {
    char *psLine = readLine(nFd);

    if (NULL == psLine || 0 == atoi(psLine)) {
        free(psLine);
        return 0;
    }
    pstShip->nVoyageId = atoi(strtok(psLine, " "));
    pstShip->psObject = copyText(strtok(NULL, " "));
    pstShip->psDestination = copyText(strtok(NULL, " "));
    pstShip->nReward = atoi(strtok(NULL, " "));
    if (-1 == asprintf(&pstShip->psObjectPath, "%s/%s", pstShip->pstConfig->psFolder, strtok(NULL, " ")))
        pstShip->psObjectPath = NULL;
    pstShip->nDelivered = atoi(strtok(NULL, " "));
    free(psLine);
    if (NULL == pstShip->psObject || NULL == pstShip->psDestination || NULL == pstShip->psObjectPath)
        return -1;
    return 0;
}
/************************************************
 * @Name: writeNumber
 * @Def: Writes a number in its own line.
 * @Arg: In: nFd = the state file
 *       In: nNumber = the number
 * @Ret: -
 ************************************************/
static void writeNumber(int nFd, int nNumber) {
    char *psText = NULL;

    if (-1 != asprintf(&psText, "%d\n", nNumber)) {
        writeText(nFd, psText);
        free(psText);
    }
}
/************************************************
 * @Name: writeMap
 * @Def: Writes the known islands ("<NAME> <IP> <PORT>") and then the
 *       routes between them ("<FROM> <TO>"), each list after its size.
 * @Arg: In: nFd = the state file
 *       In: pstMap = the ship's map
 * @Ret: -
 ************************************************/
static void writeMap(int nFd, Map *pstMap) {
    KnownIsland *pstIsland = NULL;
    char *psText = NULL;
    int nRoutes = 0;

    writeNumber(nFd, pstMap->nNumIslands);
    for (int i = 0; i < pstMap->nNumIslands; i++) {
        pstIsland = &pstMap->pstIslands[i];
        nRoutes += pstIsland->nNumRoutes;
        if (-1 != asprintf(&psText, "%s %s %d\n", pstIsland->psName, pstIsland->psIp, pstIsland->nPort)) {
            writeText(nFd, psText);
            free(psText);
        }
    }
    writeNumber(nFd, nRoutes);
    for (int i = 0; i < pstMap->nNumIslands; i++) {
        pstIsland = &pstMap->pstIslands[i];
        for (int j = 0; j < pstIsland->nNumRoutes; j++) {
            if (-1 != asprintf(&psText, "%s %s\n", pstIsland->psName, pstIsland->ppsRoutes[j])) {
                writeText(nFd, psText);
                free(psText);
            }
        }
    }
}

/************************************************
 * @Name: readCount
 * @Def: Reads a line with a number.
 * @Arg: In: nFd = the state file
 * @Ret: The number, or 0 if the line is missing.
 ************************************************/
static int readCount(int nFd) {
    char *psLine = readLine(nFd);
    int nCount = 0;

    if (NULL != psLine)
        nCount = atoi(psLine);
    free(psLine);
    return nCount;
}

/************************************************
 * @Name: readIslands
 * @Def: Reads the islands written by writeMap and adds them to the map.
 * @Arg: In: nFd = the state file
 *       In/Out: pstMap = the ship's map
 * @Ret: 0 on success, -1 if memory runs out.
 ************************************************/
static int readIslands(int nFd, Map *pstMap) {
    char *psLine = NULL;
    char *psName = NULL;
    char *psIp = NULL;
    int nPort = 0;
    int nCount = readCount(nFd);

    for (int i = 0; i < nCount; i++) {
        psLine = readLine(nFd);
        if (NULL == psLine)
            return 0;
        psName = strtok(psLine, " ");
        psIp = strtok(NULL, " ");
        nPort = atoi(strtok(NULL, " "));
        if (NULL == addIsland(pstMap, psName, psIp, nPort)) {
            free(psLine);
            return -1;
        }
        free(psLine);
    }
    return 0;
}

/************************************************
 * @Name: readRoutes
 * @Def: Reads the routes written by writeMap and charts them.
 * @Arg: In: nFd = the state file
 *       In/Out: pstMap = the ship's map, with its islands already read
 * @Ret: -
 ************************************************/
static void readRoutes(int nFd, Map *pstMap) {
    KnownIsland *pstTo = NULL;
    char *psLine = NULL;
    char *psFrom = NULL;
    int nCount = readCount(nFd);

    for (int i = 0; i < nCount; i++) {
        psLine = readLine(nFd);
        if (NULL == psLine)
            return;
        psFrom = strtok(psLine, " ");
        pstTo = findIsland(pstMap, strtok(NULL, " "));
        if (NULL != pstTo)
            chartRoute(pstMap, psFrom, pstTo->psName, pstTo->psIp, pstTo->nPort);
        free(psLine);
    }
}
/************************************************
 * @Name: readLocation
 * @Def: Reads where the ship was. Next to an island it comes back at sea,
 *       not connected: SAIL reconnects. Otherwise it stays in Ithaca.
 * @Arg: In: nFd = the state file
 *       In/Out: pstShip = the ship, in Ithaca's harbor until now
 * @Ret: -
 ************************************************/
static void readLocation(int nFd, Ship *pstShip) {
    char *psLine = readLine(nFd);
    KnownIsland *pstIsland = NULL;

    if (NULL == psLine)
        return;
    pstIsland = findIsland(&pstShip->stMap, psLine);
    if (NULL != pstIsland) {
        pstShip->psLocation = pstIsland->psName;
        pstShip->nState = SHIP_AT_SEA;
    }
    free(psLine);
}
/************************************************
 * @Name: saveState
 * @Def: Saves the ship in STATE_FILE, inside the ship's folder.
 * @Arg: In: pstShip = the ship
 * @Ret: -
 ************************************************/
void saveState(Ship *pstShip) {
    OdysseusConfig *pstConfig = pstShip->pstConfig;
    int nFd = -1;

    mkdir(pstConfig->psFolder, 0755);
    nFd = openState(pstConfig, O_WRONLY | O_CREAT | O_TRUNC);
    if (-1 == nFd) {
        printError("Error: could not save the state of the ship.\n");
        return;
    }
    writeOdysseusStock(nFd, pstConfig, pstShip->nGold);
    writeVoyage(nFd, pstShip);
    writeMap(nFd, &pstShip->stMap);
    writeText(nFd, pstShip->psLocation);
    writeText(nFd, "\n");
    close(nFd);
}

/************************************************
 * @Name: loadState
 * @Def: Recovers what saveState saved in a previous execution. With no
 *       STATE_FILE yet (first execution) the ship keeps the config values.
 * @Arg: In/Out: pstShip = the ship, just initialized
 * @Ret: 0 on success or if there was no saved state, -1 on error.
 ************************************************/
int loadState(Ship *pstShip) {
    int nFd = openState(pstShip->pstConfig, O_RDONLY);
    int nResult = 0;

    if (-1 == nFd)
        return 0;
    if (-1 == readOdysseusStock(nFd, pstShip->pstConfig) || -1 == readVoyage(nFd, pstShip) ||
        -1 == readIslands(nFd, &pstShip->stMap))
        nResult = -1;
    else {
        readRoutes(nFd, &pstShip->stMap);
        readLocation(nFd, pstShip);
    }
    pstShip->nGold = pstShip->pstConfig->nGold;
    close(nFd);
    return nResult;
}