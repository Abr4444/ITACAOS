#ifndef __ITHACA_CONFIG_H__
#define __ITHACA_CONFIG_H__

/************************************************
 * @File: ithacaConfig.h
 * @Purpose: Loads and frees the configuration of Ithaca (ithaca.dat)
 *           and its voyages (voyages.dat).
 * @Date: 28/09/2026
 ************************************************/

#include "io.h"


#define VOYAGE_AVAILABLE 0
#define VOYAGE_ASSIGNED 1
#define VOYAGE_FAILED 2

typedef struct {
    int nId;
    char *psObject;
    char *psFile;
    char *psDestination;
    int nReward;
    int nState;
    char *psHolder; // name of the Odysseus it was assigned to, or NULL
} Voyage;

typedef struct {
    char *psName;
    char *psFolder;
    char *psIp;
    int nPort;
    int nNumVoyages;
    Voyage *pstVoyages;
} IthacaConfig;

int loadIthacaConfig(char *psConfigPath, char *psVoyagesPath, IthacaConfig *pstConfig);
void freeIthacaConfig(IthacaConfig *pstConfig);

#endif