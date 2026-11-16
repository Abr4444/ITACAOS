#ifndef __SAILING_H__
#define __SAILING_H__

/************************************************
 * @File: sailing.h
 * @Purpose: SAIL: leaving a port, sailing along a known route, docking,
 *           and waiting offshore while the port is full.
 * @Date: 31/10/2026
 ************************************************/

#include <sys/select.h>
#include "ship.h"

#define RATION_KG 7               // eaten for every full RATION_SECONDS of waiting
#define RATION_SECONDS 5

// what happened while waiting offshore
#define WAIT_GOING_ON -1 // keep waiting
#define WAIT_FAILED 0 // lost at sea, connection lost or CTRL+C
#define WAIT_DOCKED 1 // the island sent DOCKED

void sail(Ship *pstShip, char *psName);
void leavePort(Ship *pstShip);

#endif