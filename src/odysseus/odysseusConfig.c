/************************************************
 * @File: odysseusConfig.c
 * @Purpose: Loads and frees the configuration of an Odysseus process
 *           (odysseus.dat).
 * @Date: 28/09/2026
 ************************************************/
#define _GNU_SOURCE
#include "odysseusConfig.h"

/************************************************
 * @Name: readStock
 * @Def: Reads the gold, the number of foods and one "<FOOD> <KG>" line
 *       per food. odysseus.dat and the saved state end with this format.
 * @Arg: In: nFd = file positioned at the gold line
 *       Out: pstConfig = gets nGold, nNumCargo and pstCargo
 * @Ret: 0 on success, -1 if memory runs out (the cargo is left empty).
 ************************************************/
static int readStock(int nFd, OdysseusConfig *pstConfig) {
    char *psLine = NULL;

    psLine = readLine(nFd);
    pstConfig->nGold = atoi(psLine);
    free(psLine);

    psLine = readLine(nFd);
    pstConfig->nNumCargo = atoi(psLine);
    free(psLine);

    if (0 < pstConfig->nNumCargo) {
        pstConfig->pstCargo = malloc(pstConfig->nNumCargo * sizeof(Cargo));
        if (NULL == pstConfig->pstCargo) {
            printError("Error: not enough memory for the cargo.\n");
            pstConfig->nNumCargo = 0;
            return -1;
        }
    }
    for (int i = 0; i < pstConfig->nNumCargo; i++) {
        pstConfig->pstCargo[i].psName = readLine(nFd);
        strtok(pstConfig->pstCargo[i].psName, " ");
        pstConfig->pstCargo[i].nKg = atoi(strtok(NULL, " "));
    }

    return 0;
}

/************************************************
 * @Name: loadOdysseusConfig
 * @Def: Reads odysseus.dat into an OdysseusConfig. The file format is
 *       assumed correct (p.18).
 * @Arg: In: psPath = path of the config file
 *       Out: pstConfig = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated).
 ************************************************/
int loadOdysseusConfig(char *psPath, OdysseusConfig *pstConfig) {
    int nFd = 0;

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

    if (-1 == readStock(nFd, pstConfig)) {
        freeOdysseusConfig(pstConfig);
        close(nFd);
        return -1;
    }
    close(nFd);
    return 0;
}
/************************************************
 * @Name: freeCargo
 * @Def: Frees the cargo and leaves the hold empty.
 * @Arg: In/Out: pstConfig = configuration with the cargo
 * @Ret: -
 ************************************************/
static void freeCargo(OdysseusConfig *pstConfig) {
    for (int i = 0; i < pstConfig->nNumCargo; i++)
        free(pstConfig->pstCargo[i].psName);
    free(pstConfig->pstCargo);
    pstConfig->pstCargo = NULL;
    pstConfig->nNumCargo = 0;
}
/************************************************
 * @Name: readOdysseusStock
 * @Def: Reads the gold and cargo saved in the state file, which replace
 *       the ones of odysseus.dat.
 * @Arg: In: nFd = state file positioned at the gold line
 *       In/Out: pstConfig = the loaded configuration
 * @Ret: 0 on success, -1 if memory runs out.
 ************************************************/
int readOdysseusStock(int nFd, OdysseusConfig *pstConfig) {
    freeCargo(pstConfig);
    return readStock(nFd, pstConfig);
}

/************************************************
 * @Name: writeText
 * @Def: Writes a string to a file.
 * @Arg: In: nFd = the file
 *       In: psText = string to write
 * @Ret: -
 ************************************************/
void writeText(int nFd, char *psText) {
    write(nFd, psText, strlen(psText));
}

/************************************************
 * @Name: writeOdyddeusStock
 * @Def: Writes the gold and cargo with the same format as the end of odysseus.dat
 * @Arg: In: nFd = the state file
 *       In: pstConfig = the configuration with the cargo
 *       In: nGold = the gold of the ship
 * @Ret: -
 ************************************************/
void writeOdysseusStock(int nFd, OdysseusConfig *pstConfig, int nGold) {
    char *psText = NULL;

    if (-1 != asprintf(&psText, "%d\n%d\n", nGold, pstConfig->nNumCargo)) {
        writeText(nFd, psText);
        free(psText);
    }
    for (int i = 0; i < pstConfig->nNumCargo; i++) {
        if (-1 != asprintf(&psText, "%s %d\n", pstConfig->pstCargo[i].psName, pstConfig->pstCargo[i].nKg)) {
            writeText(nFd, psText);
            free(psText);
        }
    }
}
/************************************************
 * @Name: freeOdysseusConfig
 * @Def: Frees everything loadOdysseusConfig allocated.
 * @Arg: In/Out: pstConfig = configuration to free
 * @Ret: -
 ************************************************/
void freeOdysseusConfig(OdysseusConfig *pstConfig) {
    free(pstConfig->psName);
    free(pstConfig->psFolder);
    free(pstConfig->psIthacaIp);
    free(pstConfig->psAeaeaName);
    freeCargo(pstConfig);
}