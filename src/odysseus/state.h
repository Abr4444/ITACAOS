#ifndef __STATE_H__
#define __STATE_H__

/************************************************
 * @File: state.h
 * @Purpose: Saves the ship in its folder after every command and
 *           recovers it when Odysseus starts again.
 * @Date: 21/11/2026
 ************************************************/

#include <sys/stat.h>
#include "ship.h"

#define STATE_FILE "state.dat"

int loadState(Ship *pstShip);
void saveState(Ship *pstShip);
void deleteState(Ship *pstShip);

#endif