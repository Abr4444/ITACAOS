/************************************************
 *
 * @File: odysseus.c
 * @Purpose: Odysseus process (the ship). Usage: ./odysseus <config.dat>
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/
#define _GNU_SOURCE
#include "odysseusConfig.h"
#include "io.h"


int main(int argc, char *argv[]) {

    OdysseusConfig config;
    char *message = NULL;

    if (2 != argc) {
        printError("Usage: ./odysseus <config.dat>\n");
        return 1;
    }
    if (-1 == loadOdysseusConfig(argv[1], &config)) {
        return 1;
    }


    freeOdysseusConfig(&config);
    return 0;
}
