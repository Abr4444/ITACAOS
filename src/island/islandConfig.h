#ifndef __ISLAND_CONFIG_H__
#define __ISLAND_CONFIG_H__

/************************************************
 *
 * @File: islandConfig.h
 * @Purpose: Loads and frees the configuration of an Island process
 *           (island.dat with its routes) and its market (stock.db).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 30/09/2026
 *
 ************************************************/

#include "io.h"
#include "sphragis.h"


typedef struct {
    char *psName;
    char *psIp;
    int nPort;
} Route;

typedef struct {
    char sName[100];
    int nAmount;
    int nPrice;
} Product;

typedef struct {
    char *psName;
    char *psFolder;
    char *psStockPath;
    char *psIp;
    int nPort;
    int nCapacity;
    int nNumRoutes;
    Route *pstRoutes;
    int nNumProducts;
    Product *pstProducts;
} IslandConfig;

int loadIslandConfig(char *psConfigPath, char *psStockPath, IslandConfig *pstConfig);
void freeIslandConfig(IslandConfig *pstConfig);

#endif