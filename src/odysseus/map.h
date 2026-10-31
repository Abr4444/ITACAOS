#ifndef __MAP_H__
#define __MAP_H__

/************************************************
 *
 * @File: map.h
 * @Purpose: The ship's map: the islands it knows, where their Island
 *           processes are and the direct routes between them.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 31/10/2026
 *
 ************************************************/

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "io.h"

//an island of the ship's map: where it is and which islands it connects to
typedef struct {
    char *psName;
    char *psIp;
    int nPort;
    char **ppsRoutes;// names of the islands with a direct route from this one
    int nNumRoutes;
} KnownIsland;

typedef struct {
    KnownIsland *pstIslands;  // Aeaea first
    int nNumIslands;
} Map;

void initMap(Map *pstMap);
KnownIsland *findIsland(Map *pstMap, char *psName);
KnownIsland *addIsland(Map *pstMap, char *psName, char *psIp, int nPort);
void chartRoute(Map *pstMap, char *psFrom, char *psName, char *psIp, int nPort);
void freeMap(Map *pstMap);

#endif