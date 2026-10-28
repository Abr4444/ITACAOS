#ifndef __ISLAND_SERVER_H__
#define __ISLAND_SERVER_H__

#include "islandConfig.h"
#include "frame.h"
#include "server.h"

// Client->nState of a ship
#define SHIP_OUTSIDE 0   // connected, not arrived yet (or already departed)
#define SHIP_QUEUED 1    // waiting offshore in the FIFO queue
#define SHIP_DOCKED 2    // inside the port, owns a berth

#define MAP_PRICE 50

typedef struct {
    Server stServer;           // generic part: threads, ship list, mutex (also protects the fields below)
    IslandConfig *pstConfig;
    int nStockFd;              // stock.db, kept open to save every change
    int nDocked;               // berths in use
    int nNextTicket;           // arrival order: lowest ticket docks first
} IslandServer;

int initIslandServer(IslandServer *pstIsland, IslandConfig *pstConfig);
void closeIslandServer(IslandServer *pstIsland);

#endif
