#ifndef __VOYAGES_H__
#define __VOYAGES_H__

/************************************************
 * @File: voyages.h
 * @Purpose: Odysseus with Ithaca: CONNECT ITHACA, LIST VOYAGES and ACCEPT.
 * @Date: 31/10/2026
 ************************************************/

#include "ship.h"

#define LIST_VOYAGE_FIELDS 6     //Index&Total&VoyageID&Object&Destination&Reward
#define ACCEPT_VOYAGE_FIELDS 5   //OK&VoyageID&Object&Destination&Reward

void connectIthaca(Ship *pstShip);
void listVoyages(Ship *pstShip);
void acceptVoyage(Ship *pstShip, int nId);
void forgetListedVoyages(Ship *pstShip);

#endif