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
/************************************************
 *
 * @Name: printConfig
 * @Def: Prints every loaded field. Temporary: only to check the parser.
 * @Arg: In: config = configuration to print
 * @Ret: -
 *
 ************************************************/
// will get removed after testing
void printConfig(OdysseusConfig *config) {

    char *message = NULL;
    int i = 0;

    if (-1 != asprintf(&message, "Name: %s\nFolder: %s\nIthaca: %s %d\nAeaea: %s %d\nGold: %d\nCargo (%d):\n",
            config->name, config->folder, config->ithacaIp, config->ithacaPort,
            config->aeaeaIp, config->aeaeaPort, config->gold, config->numCargo)) {
        printString(message);
        free(message);
    }
    for (i = 0; i < config->numCargo; i++) {
        if (-1 != asprintf(&message, "  %s: %d kg\n", config->cargo[i].name, config->cargo[i].kg)) {
            printString(message);
            free(message);
        }
    }
}

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

    // Temporary: remove once the parser is checked
    printConfig(&config);

    if (-1 != asprintf(&message, "Odysseus %s is ready to sail.\n", config.name)) {
        printString(message);
        free(message);
    }

    freeOdysseusConfig(&config);
    return 0;
}
