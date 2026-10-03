#ifndef __ITHACA_CONFIG_H__
#define __ITHACA_CONFIG_H__

/************************************************
 *
 * @File: ithacaConfig.h
 * @Purpose: Loads and frees the configuration of Ithaca (ithaca.dat)
 *           and its voyages (voyages.dat).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

#include "io.h"

typedef struct {
    int nId;
    char *psObject;
    char *psFile;
    char *psDestination;
    int nReward;
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