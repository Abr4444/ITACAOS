#ifndef __MARKET_H__
#define __MARKET_H__

/************************************************
 *
 * @File: market.h
 * @Purpose: Odysseus docked at an island: BUY MAP (and the market commands).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 31/10/2026
 *
 ************************************************/

#include "ship.h"

#define BUY_MAP_FIELDS 7         //OK&Price&Index&Total&Name&IP&Port

void buyMap(Ship *pstShip);

#endif