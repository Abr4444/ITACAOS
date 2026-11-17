/************************************************
 * @File: islandConfig.c
 * @Purpose: Loads and frees the configuration of an Island process
 *           (island.dat with its routes) and its market (stock.db).
 * @Date: 30/09/2026
 ************************************************/

#include "islandConfig.h"

/* @Name: readRoutes
 * @Def: Reads the "<NAME> <IP> <PORT>" lines after "--- ROUTES ---".
 * @Arg: In: nFd = file descriptor of island.dat, positioned at the routes
 *       Out: pstConfig = gets pstRoutes and nNumRoutes
 * @Ret: 0 on success, -1 if memory runs out. */
static int readRoutes(int nFd, IslandConfig *pstConfig) {

    Route *pstMore = NULL;
    char *psLine = NULL;
    int nIndex = 0;

    psLine = readLine(nFd);
    while (NULL != psLine && '\0' != psLine[0]) {
        pstMore = realloc(pstConfig->pstRoutes, (pstConfig->nNumRoutes + 1) * sizeof(Route));
        if (NULL == pstMore) {
            printError("Error: not enough memory for the routes.\n");
            free(psLine);
            return -1;
        }
        pstConfig->pstRoutes = pstMore;

        nIndex = pstConfig->nNumRoutes;
        pstConfig->pstRoutes[nIndex].psName = psLine;
        strtok(psLine, " ");
        pstConfig->pstRoutes[nIndex].psIp = strtok(NULL, " ");
        pstConfig->pstRoutes[nIndex].nPort = atoi(strtok(NULL, " "));
        pstConfig->nNumRoutes++;

        psLine = readLine(nFd);
    }
    free(psLine);
    return 0;
}

/* @Name: filterRoutes
 * @Def: Removes the routes SPHRAGIS rejects.
 * @Arg: In/Out: pstConfig = its routes are reduced to the valid ones
 * @Ret: 0 on success, -1 on error. */
static int filterRoutes(IslandConfig *pstConfig) {

    SPHRAGIS_Island stIsland;
    int nCount = 0;
    int nKept = 0;
    int i = 0;

    stIsland.name = pstConfig->psName;
    stIsland.known_island_count = pstConfig->nNumRoutes;
    stIsland.known_islands = NULL;
    if (0 < pstConfig->nNumRoutes) {
        stIsland.known_islands = malloc(pstConfig->nNumRoutes * sizeof(char *));
        if (NULL == stIsland.known_islands) {
            printError("Error: not enough memory to validate the routes.\n");
            return -1;
        }
    }
    for (i = 0; i < pstConfig->nNumRoutes; i++) {
        stIsland.known_islands[i] = pstConfig->pstRoutes[i].psName;
    }

    nCount = SPHRAGIS_filter_island_configuration(&stIsland);
    if (0 > nCount) {
        printError("Error: SPHRAGIS rejected the island configuration.\n");
        free(stIsland.known_islands);
        return -1;
    }

    // Survivors keep their order, so one pass keeps exactly the routes still in the list
    for (i = 0; i < pstConfig->nNumRoutes && nKept < nCount; i++) {
        if (pstConfig->pstRoutes[i].psName == stIsland.known_islands[nKept]) {
            pstConfig->pstRoutes[nKept] = pstConfig->pstRoutes[i];
            nKept++;
        }
    }
    pstConfig->nNumRoutes = nCount;
    free(stIsland.known_islands);
    return 0;
}

/* @Name: loadStock
 * @Def: Reads every product record of stock.db.
 * @Arg: In: psPath = path of stock.db
 *       Out: pstConfig = gets pstProducts and nNumProducts
 * @Ret: 0 on success, -1 on error. */
static int loadStock(char *psPath, IslandConfig *pstConfig) {

    Product stProduct;
    Product *pstMore = NULL;
    int nSize = sizeof(Product);
    int nFd = 0;

    nFd = open(psPath, O_RDONLY);
    if (-1 == nFd) {
        printError("Error: could not open the stock file.\n");
        return -1;
    }

    while (nSize == read(nFd, &stProduct, nSize)) {
        pstMore = realloc(pstConfig->pstProducts, (pstConfig->nNumProducts + 1) * sizeof(Product));
        if (NULL == pstMore) {
            printError("Error: not enough memory for the stock.\n");
            close(nFd);
            return -1;
        }
        pstConfig->pstProducts = pstMore;
        pstConfig->pstProducts[pstConfig->nNumProducts] = stProduct;
        pstConfig->nNumProducts++;
    }

    close(nFd);
    return 0;
}

/* @Name: loadIslandConfig
 * @Def: Reads island.dat, filters its routes and loads stock.db.
 * @Arg: In: psConfigPath = path of island.dat
 *       In: psStockPath = path of stock.db
 *       Out: pstConfig = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated). */
int loadIslandConfig(char *psConfigPath, char *psStockPath, IslandConfig *pstConfig) {

    char *psLine = NULL;
    int nFd = 0;

    memset(pstConfig, 0, sizeof(IslandConfig));

    nFd = open(psConfigPath, O_RDONLY);
    if (-1 == nFd) {
        printError("Error: could not open the island config file.\n");
        return -1;
    }
    pstConfig->psStockPath = copyText(psStockPath);
    if (NULL == pstConfig->psStockPath) {
        printError("Error: not enough memory for the stock path.\n");
        close(nFd);
        return -1;
    }
    pstConfig->psName = readLine(nFd);
    pstConfig->psFolder = readLine(nFd);
    pstConfig->psIp = readLine(nFd);
    strtok(pstConfig->psIp, " ");
    pstConfig->nPort = atoi(strtok(NULL, " "));
    psLine = readLine(nFd);
    pstConfig->nCapacity = atoi(psLine);
    free(psLine);
    free(readLine(nFd));

    if (-1 == readRoutes(nFd, pstConfig) || -1 == filterRoutes(pstConfig) ||
        -1 == loadStock(psStockPath, pstConfig)) {
        close(nFd);
        freeIslandConfig(pstConfig);
        return -1;
    }
    close(nFd);
    return 0;
}

/* @Name: freeIslandConfig
 * @Def: Frees everything loadIslandConfig allocated.
 * @Arg: In/Out: pstConfig = configuration to free
 * @Ret: - */
void freeIslandConfig(IslandConfig *pstConfig) {

    int i = 0;

    free(pstConfig->psName);
    free(pstConfig->psFolder);
    free(pstConfig->psStockPath);
    free(pstConfig->psIp);
    for (i = 0; i < pstConfig->nNumRoutes; i++) {
        free(pstConfig->pstRoutes[i].psName);
    }
    free(pstConfig->pstRoutes);
    free(pstConfig->pstProducts);
}