#ifndef __DELIVERY_H__
#define __DELIVERY_H__

/************************************************
 * @File: delivery.h
 * @Purpose: The end of a voyage: DELIVER at the destination island and CLAIM at Ithaca.
 * @Date: 21/11/2026
 ************************************************/

#include "ship.h"
#include "market.h"
#include "voyages.h"

#define CLAIM_FIELDS 2 //Ok&Reward

void deliver(Ship *pstShip);

void claim(Ship* pstShip);

#endif