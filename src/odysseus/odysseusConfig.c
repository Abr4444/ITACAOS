/************************************************
 * @File: odysseusConfig.c
 * @Purpose: Loads and frees the configuration of an Odysseus process
 *           (odysseus.dat).
 * @Date: 28/09/2026
 ************************************************/

#include "odysseusConfig.h"

/************************************************
 * @Name: loadOdysseusConfig
 * @Def: Reads odysseus.dat into an OdysseusConfig. The file format is
 *       assumed correct (p.18).
 * @Arg: In: psPath = path of the config file
 *       Out: pstConfig = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated).
 ************************************************/
int loadOdysseusConfig(char *psPath, OdysseusConfig *pstConfig) {

    char *psLine = NULL;
    int nFd = 0;
    int i = 0;

    memset(pstConfig, 0, sizeof(OdysseusConfig));

    nFd = open(psPath, O_RDONLY);
    if (-1 == nFd) {
        printError("Error: could not open the Odysseus config file.\n");
        return -1;
    }

    pstConfig->psName = readLine(nFd);
    pstConfig->psFolder = readLine(nFd);

    pstConfig->psIthacaIp = readLine(nFd);
    strtok(pstConfig->psIthacaIp, " ");
    pstConfig->nIthacaPort = atoi(strtok(NULL, " "));

    pstConfig->psAeaeaName = readLine(nFd);
    strtok(pstConfig->psAeaeaName, " ");
    pstConfig->psAeaeaIp = strtok(NULL, " ");
    pstConfig->nAeaeaPort = atoi(strtok(NULL, " "));

    psLine = readLine(nFd);
    pstConfig->nGold = atoi(psLine);
    free(psLine);

    psLine = readLine(nFd);
    pstConfig->nNumCargo = atoi(psLine);
    free(psLine);

    // malloc(0) may return NULL, so an empty hold allocates nothing
    if (0 < pstConfig->nNumCargo) {
        pstConfig->pstCargo = malloc(pstConfig->nNumCargo * sizeof(Cargo));
        if (NULL == pstConfig->pstCargo) {
            printError("Error: not enough memory for the cargo.\n");
            pstConfig->nNumCargo = 0;
            freeOdysseusConfig(pstConfig);
            close(nFd);
            return -1;
        }
    }
    for (i = 0; i < pstConfig->nNumCargo; i++) {
        pstConfig->pstCargo[i].psName = readLine(nFd);
        strtok(pstConfig->pstCargo[i].psName, " ");
        pstConfig->pstCargo[i].nKg = atoi(strtok(NULL, " "));
    }

    close(nFd);
    return 0;
}

/************************************************
 * @Name: freeOdysseusConfig
 * @Def: Frees everything loadOdysseusConfig allocated.
 * @Arg: In/Out: pstConfig = configuration to free
 * @Ret: -
 ************************************************/
void freeOdysseusConfig(OdysseusConfig *pstConfig) {

    int i = 0;

    free(pstConfig->psName);
    free(pstConfig->psFolder);
    free(pstConfig->psIthacaIp);
    free(pstConfig->psAeaeaName);
    for (i = 0; i < pstConfig->nNumCargo; i++) {
        free(pstConfig->pstCargo[i].psName);
    }
    free(pstConfig->pstCargo);
}