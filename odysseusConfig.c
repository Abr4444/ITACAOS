/************************************************
 *
 * @File: odysseusConfig.c
 * @Purpose: Loads and frees the configuration of an Odysseus process
 *           (odysseus.dat).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

#include "odysseusConfig.h"

/************************************************
 *
 * @Name: loadOdysseusConfig
 * @Def: Reads odysseus.dat into an OdysseusConfig. The file format is
 *       assumed correct (p.18).
 * @Arg: In: path = path of the config file
 *       Out: config = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated).
 *
 ************************************************/
int loadOdysseusConfig(char *path, OdysseusConfig *config) {

    char *line = NULL;
    int fd = 0;
    int i = 0;

    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        printError("Error: could not open the Odysseus config file.\n");
        return -1;
    }

    config->name = readLine(fd);
    config->folder = readLine(fd);

    config->ithacaIp = readLine(fd);
    strtok(config->ithacaIp, " ");
    config->ithacaPort = atoi(strtok(NULL, " "));

    config->aeaeaName = readLine(fd);
    strtok(config->aeaeaName, " ");
    config->aeaeaIp = strtok(NULL, " ");
    config->aeaeaPort = atoi(strtok(NULL, " "));

    line = readLine(fd);
    config->gold = atoi(line);
    free(line);

    line = readLine(fd);
    config->numCargo = atoi(line);
    free(line);

    config->cargo = malloc(config->numCargo * sizeof(Cargo));
    if (NULL == config->cargo) {
        printError("Error: not enough memory for the cargo.\n");
        config->numCargo = 0;
        freeOdysseusConfig(config);
        close(fd);
        return -1;
    }
    for (i = 0; i < config->numCargo; i++) {
        config->cargo[i].name = readLine(fd);
        strtok(config->cargo[i].name, " ");
        config->cargo[i].kg = atoi(strtok(NULL, " "));
    }

    close(fd);
    return 0;
}

/************************************************
 *
 * @Name: freeOdysseusConfig
 * @Def: Frees everything loadOdysseusConfig allocated.
 * @Arg: In/Out: config = configuration to free
 * @Ret: -
 *
 ************************************************/
void freeOdysseusConfig(OdysseusConfig *config) {

    int i = 0;

    free(config->name);
    free(config->folder);
    free(config->ithacaIp);
    free(config->aeaeaName);
    for (i = 0; i < config->numCargo; i++) {
        free(config->cargo[i].name);
    }
    free(config->cargo);
}