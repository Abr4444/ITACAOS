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
    int id;               // line number in voyages.dat, starting at 1
    char *object;         // the whole line after strtok; file and destination point inside it
    char *file;
    char *destination;
    int reward;
} Voyage;

typedef struct {
    char *name;
    char *folder;
    char *ip;
    int port;
    int numVoyages;
    Voyage *voyages;
} IthacaConfig;

int loadIthacaConfig(char *configPath, char *voyagesPath, IthacaConfig *config);
void freeIthacaConfig(IthacaConfig *config);

#endif