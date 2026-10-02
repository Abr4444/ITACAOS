/************************************************
 *
 * @File: interrupt.c
 * @Purpose: CTRL+C (SIGINT) handling shared by Odysseus, Ithaca and Island.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 02/10/2026
 *
 ************************************************/

#include "interrupt.h"

volatile sig_atomic_t gnStop = 0;

/************************************************
 *
 * @Name: handleInterrupt
 * @Def: SIGINT handler: only raises the stop flag, main does the cleanup.
 * @Arg: In: nSignum = signal received
 * @Ret: -
 *
 ************************************************/
static void handleInterrupt(int nSignum) {

    (void) nSignum;
    gnStop = 1;
}

/************************************************
 *
 * @Name: setUpInterrupt
 * @Def: Installs the SIGINT handler without SA_RESTART, so a blocking
 *       read() or pause() returns -1 (EINTR) and the caller can exit cleanly.
 * @Arg: -
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
int setUpInterrupt(void) {

    struct sigaction stAction;

    memset(&stAction, 0, sizeof(stAction));
    stAction.sa_handler = handleInterrupt;
    sigemptyset(&stAction.sa_mask);
    // No SA_RESTART: blocked syscalls must be interrupted by CTRL+C
    stAction.sa_flags = 0;
    return sigaction(SIGINT, &stAction, NULL);
}