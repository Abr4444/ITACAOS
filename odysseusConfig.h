#ifndef __ODYSSEUS_CONFIG_H__
#define __ODYSSEUS_CONFIG_H__

/************************************************
 *
 * @File: odysseusConfig.h
 * @Purpose: Loads and frees the configuration of an Odysseus process
 *           (odysseus.dat).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/


#include "io.h"

typedef struct {
    char *name;
    int kg;
} Cargo;

typedef struct {
    char *name;
    char *folder;
    char *ithacaIp;
    int ithacaPort;
    char *aeaeaName;      // the whole "Aeaea <IP> <PORT>" line after strtok
    char *aeaeaIp;        // points inside aeaeaName's memory, never freed on its own
    int aeaeaPort;
    int gold;
    int numCargo;
    Cargo *cargo;
} OdysseusConfig;

int loadOdysseusConfig(char *path, OdysseusConfig *config);
void freeOdysseusConfig(OdysseusConfig *config);

#endif