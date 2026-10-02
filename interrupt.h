#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

/************************************************
 *
 * @File: interrupt.h
 * @Purpose: CTRL+C (SIGINT) handling shared by Odysseus, Ithaca and Island.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/

// System includes
#include <signal.h>
#include <string.h>

// Raised by the SIGINT handler; the main loops check it and clean up
extern volatile sig_atomic_t gnStop;

int setUpInterrupt(void);

#endif