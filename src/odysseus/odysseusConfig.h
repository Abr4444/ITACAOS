#ifndef __ODYSSEUS_CONFIG_H__
#define __ODYSSEUS_CONFIG_H__

/************************************************
 * @File: odysseusConfig.h
 * @Purpose: Loads and frees the configuration of an Odysseus process
 *           (odysseus.dat).
 * @Date: 28/09/2026
 ************************************************/
#include "io.h" 

typedef struct {
    char *psName;
    int nKg;
} Cargo;

typedef struct {
    char *psName;
    char *psFolder;
    char *psIthacaIp;
    int nIthacaPort;
    char *psAeaeaName;
    char *psAeaeaIp;
    int nAeaeaPort;
    int nGold;
    int nNumCargo;
    Cargo *pstCargo;
} OdysseusConfig;

int loadOdysseusConfig(char *psPath, OdysseusConfig *pstConfig);
void freeOdysseusConfig(OdysseusConfig *pstConfig);
int readOdysseusStock(int nFd, OdysseusConfig *pstConfig);
void writeOdysseusStock(int nFd, OdysseusConfig *pstConfig, int nGold);
void writeText(int nFd, char *psText);

#endif