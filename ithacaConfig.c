/************************************************
 *
 * @File: ithacaConfig.c
 * @Purpose: Loads and frees the configuration of Ithaca (ithaca.dat)
 *           and its voyages (voyages.dat).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

#include "ithacaConfig.h"

/************************************************
 *
 * @Name: loadVoyages
 * @Def: Reads every "<OBJECT> <FILE> <DESTINATION> <REWARD>" line of
 *       voyages.dat and gives each voyage its line number as ID.
 * @Arg: In: psPath = path of voyages.dat
 *       Out: pstConfig = gets pstVoyages and nNumVoyages
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
static int loadVoyages(char *psPath, IthacaConfig *pstConfig) {

    Voyage *pstMore = NULL;
    char *psLine = NULL;
    int nFd = 0;
    int nIndex = 0;

    nFd = open(psPath, O_RDONLY);
    if (-1 == nFd) {
        printError("Error: could not open the voyages file.\n");
        return -1;
    }

    psLine = readLine(nFd);
    while (NULL != psLine && '\0' != psLine[0]) {
        pstMore = realloc(pstConfig->pstVoyages, (pstConfig->nNumVoyages + 1) * sizeof(Voyage));
        if (NULL == pstMore) {
            printError("Error: not enough memory for the voyages.\n");
            free(psLine);
            close(nFd);
            return -1;
        }
        pstConfig->pstVoyages = pstMore;

        nIndex = pstConfig->nNumVoyages;
        pstConfig->pstVoyages[nIndex].nId = nIndex + 1;
        pstConfig->pstVoyages[nIndex].psObject = psLine;
        strtok(psLine, " ");
        pstConfig->pstVoyages[nIndex].psFile = strtok(NULL, " ");
        pstConfig->pstVoyages[nIndex].psDestination = strtok(NULL, " ");
        pstConfig->pstVoyages[nIndex].nReward = atoi(strtok(NULL, " "));
        pstConfig->nNumVoyages++;

        psLine = readLine(nFd);
    }

    free(psLine);
    close(nFd);
    return 0;
}

/************************************************
 *
 * @Name: loadIthacaConfig
 * @Def: Reads ithaca.dat and voyages.dat. The format is assumed correct (p.18).
 * @Arg: In: psConfigPath = path of ithaca.dat
 *       In: psVoyagesPath = path of voyages.dat
 *       Out: pstConfig = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated).
 *
 ************************************************/
int loadIthacaConfig(char *psConfigPath, char *psVoyagesPath, IthacaConfig *pstConfig) {

    int nFd = 0;

    memset(pstConfig, 0, sizeof(IthacaConfig));

    nFd = open(psConfigPath, O_RDONLY);
    if (-1 == nFd) {
        printError("Error: could not open the Ithaca config file.\n");
        return -1;
    }
    pstConfig->psName = readLine(nFd);
    pstConfig->psFolder = readLine(nFd);
    pstConfig->psIp = readLine(nFd);
    strtok(pstConfig->psIp, " ");
    pstConfig->nPort = atoi(strtok(NULL, " "));
    close(nFd);

    if (-1 == loadVoyages(psVoyagesPath, pstConfig)) {
        freeIthacaConfig(pstConfig);
        return -1;
    }
    return 0;
}

/************************************************
 *
 * @Name: freeIthacaConfig
 * @Def: Frees everything loadIthacaConfig allocated.
 * @Arg: In/Out: pstConfig = configuration to free
 * @Ret: -
 *
 ************************************************/
void freeIthacaConfig(IthacaConfig *pstConfig) {

    int i = 0;

    free(pstConfig->psName);
    free(pstConfig->psFolder);
    free(pstConfig->psIp);
    for (i = 0; i < pstConfig->nNumVoyages; i++) {
        free(pstConfig->pstVoyages[i].psObject);
    }
    free(pstConfig->pstVoyages);
}