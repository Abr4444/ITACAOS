#include "map.h"
#include "sphragis.h"


/************************************************
 *
 * @Name: initMap
 * @Def: Starts an empty map.
 * @Arg: Out: pstMap = the map
 * @Ret: -
 *
 ************************************************/
void initMap(Map *pstMap) {
    pstMap->pstIslands = NULL;
    pstMap->nNumIslands = 0;
}
/************************************************
 *
 * @Name: findIsland
 * @Def: Looks for an island in the ship's map. Island names are case
 *       insensitive, like the commands.
 * @Arg: In: pstMap = the ship's map
 *       In: psName = name of the island
 * @Ret: The island, or NULL if the ship does not know it.
 *
 ************************************************/
KnownIsland *findIsland(Map *pstMap, char *psName) {

    int i = 0;

    for (i = 0; i < pstMap->nNumIslands; i++) {
        if (0 == strcasecmp(pstMap->pstIslands[i].psName, psName))
            return &pstMap->pstIslands[i];
    }
    return NULL;
}

/************************************************
 *
 * @Name: addIsland
 * @Def: Adds an island and its address to the ship's map, if the ship
 *       does not know it yet.
 * @Arg: In/Out: pstMap = the ship's map
 *       In: psName = name of the island
 *       In: psIp = IP of its Island process
 *       In: nPort = port of its Island process
 * @Ret: The island, or NULL if there was no memory.
 *
 ************************************************/
KnownIsland *addIsland(Map *pstMap, char *psName, char *psIp, int nPort) {

    KnownIsland *pstIsland = findIsland(pstMap, psName);
    KnownIsland *pstMore = NULL;

    if (NULL != pstIsland)
        return pstIsland;
    pstMore = realloc(pstMap->pstIslands, (pstMap->nNumIslands + 1) * sizeof(KnownIsland));
    if (NULL == pstMore)
        return NULL;
    pstMap->pstIslands = pstMore;

    pstIsland = &pstMap->pstIslands[pstMap->nNumIslands];
    pstIsland->psName = copyText(psName);
    pstIsland->psIp = copyText(psIp);
    pstIsland->nPort = nPort;
    pstIsland->ppsRoutes = NULL;
    pstIsland->nNumRoutes = 0;
    if (NULL == pstIsland->psName || NULL == pstIsland->psIp) {
        free(pstIsland->psName);
        free(pstIsland->psIp);
        return NULL;
    }
    pstMap->nNumIslands++;
    return pstIsland;
}

/************************************************
 *
 * @Name: addRoute
 * @Def: Adds a direct route to an island of the map, if it is not there
 *       yet. The route stores a pointer to the other island's own name.
 * @Arg: In/Out: pstIsland = island the route starts from
 *       In: psName = name of the island it leads to (owned by that island)
 * @Ret: 0 on success, -1 if there was no memory.
 *
 ************************************************/
static int addRoute(KnownIsland *pstIsland, char *psName) {
    char **ppsMore = NULL;

    for (int i = 0; i < pstIsland->nNumRoutes; i++) {
        if (0 == strcasecmp(pstIsland->ppsRoutes[i], psName))
            return 0;
    }
    ppsMore = realloc(pstIsland->ppsRoutes, (pstIsland->nNumRoutes + 1) * sizeof(char *));
    if (NULL == ppsMore)
        return -1;
    pstIsland->ppsRoutes = ppsMore;
    pstIsland->ppsRoutes[pstIsland->nNumRoutes] = psName;
    pstIsland->nNumRoutes++;
    return 0;
}
/************************************************
 *
 * @Name: chartRoute
 * @Def: Adds a route of the map just bought to the ship's map, in both
 *       directions, because all routes are bidirectional.
 * @Arg: In/Out: pstMap = the ship's map
 *       In: psFrom = island that sold the map (where the ship is)
 *       In: psName = island the route leads to
 *       In: psIp = IP of that island
 *       In: nPort = port of that island
 * @Ret: -
 *
 ************************************************/
void chartRoute(Map *pstMap, char *psFrom, char *psName, char *psIp, int nPort) {
    KnownIsland *pstTo = addIsland(pstMap, psName, psIp, nPort);
    KnownIsland *pstFrom = findIsland(pstMap, psFrom);

    if (NULL == pstTo || NULL == pstFrom || -1 == addRoute(pstFrom, pstTo->psName) ||
        -1 == addRoute(pstTo, pstFrom->psName))
        printString("Error: not enough memory to chart the route.\n");
}

/* @Name: showMap
 * @Def: MAP: draws the known islands and routes with SPHRAGIS.
 * @Arg: In: pstMap = the ship's map (Aeaea first)
 * @Ret: - */
void showMap(Map *pstMap) {

    SPHRAGIS_Island *pstIslands = NULL;
    int nResult = 0;
    int i = 0;

    pstIslands = malloc(pstMap->nNumIslands * sizeof(SPHRAGIS_Island));
    if (NULL == pstIslands) {
        printString("Error: not enough memory to show the map.\n");
        return;
    }
    for (i = 0; i < pstMap->nNumIslands; i++) {
        pstIslands[i].name = pstMap->pstIslands[i].psName;
        pstIslands[i].known_islands = pstMap->pstIslands[i].ppsRoutes;
        pstIslands[i].known_island_count = pstMap->pstIslands[i].nNumRoutes;
    }

    nResult = SPHRAGIS_display_map(pstIslands, pstMap->nNumIslands);
    if (SPHRAGIS_OK != nResult && SPHRAGIS_ERROR != nResult) {
        printString("Error: SPHRAGIS could not show the map.\n");
    }
    free(pstIslands);
}

/************************************************
 *
 * @Name: freeMap
 * @Def: Frees the map. ppsRoutes only points to the names of other
 *       islands, so each name is freed once, with its own island.
 * @Arg: In/Out: pstMap = the map
 * @Ret: -
 *
 ************************************************/
void freeMap(Map *pstMap) {
    for (int i = 0; i < pstMap->nNumIslands; i++) {
        free(pstMap->pstIslands[i].psName);
        free(pstMap->pstIslands[i].psIp);
        free(pstMap->pstIslands[i].ppsRoutes);
    }
    free(pstMap->pstIslands);
    pstMap->pstIslands = NULL;
    pstMap->nNumIslands = 0;
}