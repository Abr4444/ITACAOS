#ifndef __MARKET_H__
#define __MARKET_H__

/************************************************
 *
 * @File: market.h
 * @Purpose: Odysseus docked at an island: LIST MARKET, BUY, SELL and BUY MAP.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 31/10/2026
 *
 ************************************************/

#include "ship.h"

#define BUY_MAP_FIELDS 7         //OK&Price&Index&Total&Name&IP&Port
#define MARKET_FIELDS 5          //Index&Total&Product&Amount&Price
#define TRADE_FIELDS 4           //OK&Product&Amount&Cost (or Income)

void listMarket(Ship *pstShip);
void buy(Ship *pstShip, char *psProduct, int nAmount);
void sell(Ship *pstShip, char *psProduct, int nAmount);
void buyMap(Ship *pstShip);

#endif