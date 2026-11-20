#ifndef __ISLAND_SERVER_H__
#define __ISLAND_SERVER_H__

/************************************************
 * @File: islandServer.h
 * @Purpose: An island's server: serves each Odysseus in its own thread,
 *           with the berths of the port, the FIFO queue of ships waiting
 *           offshore and the market (kept in stock.db).
 * @Date: 30/10/2026
 ************************************************/

#include "islandConfig.h"
#include "frame.h"
#include "server.h"
#include "transfer.h"

// Where a ship is (Client nState)
#define SHIP_OUTSIDE 0   // connected, not arrived yet (or already departed)
#define SHIP_QUEUED 1    // waiting offshore in the FIFO queue
#define SHIP_DOCKED 2    // inside the port, owns a berth

#define DELIVER_FIELDS 2 // <VoyageID>&<Object>


typedef struct {
    ClientList stShips;        // connected ships; its mutex also protects the fields below
    IslandConfig *pstConfig;
    int nStockFd;              // stock.db, kept open to save every change
    int nDocked;               // berths in use
    int nNextTicket;           // arrival order: lowest ticket docks first
} IslandServer;

int initIslandServer(IslandServer *pstIsland, IslandConfig *pstConfig);
void runIslandServer(IslandServer *pstIsland, int nListenFd);
void closeIslandServer(IslandServer *pstIsland);

#endif
