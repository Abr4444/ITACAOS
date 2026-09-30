/************************************************
 *
 * @File: ithacaConfig.c
 * @Purpose: Loads and frees the configuration of Ithaca (ithaca.dat)
 *           and its voyages (voyages.dat).
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

#include "ithacaConfig.h"

/************************************************
 *
 * @Name: loadVoyages
 * @Def: Reads every "<OBJECT> <FILE> <DESTINATION> <REWARD>" line of
 *       voyages.dat and gives each voyage its line number as ID.
 * @Arg: In: path = path of voyages.dat
 *       Out: config = gets voyages and numVoyages
 * @Ret: 0 on success, -1 on error.
 *
 ************************************************/
static int loadVoyages(char *path, IthacaConfig *config) {

    Voyage *more = NULL;
    char *line = NULL;
    int fd = 0;
    int n = 0;

    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        printError("Error: could not open the voyages file.\n");
        return -1;
    }

    line = readLine(fd);
    while (NULL != line) {
        // Grow the array by one; a temporary pointer keeps the old array if realloc fails
        more = realloc(config->voyages, (config->numVoyages + 1) * sizeof(Voyage));
        if (NULL == more) {
            printError("Error: not enough memory for the voyages.\n");
            free(line);
            close(fd);
            return -1;
        }
        config->voyages = more;

        n = config->numVoyages;
        config->voyages[n].id = n + 1;
        config->voyages[n].object = line;
        strtok(line, " ");
        config->voyages[n].file = strtok(NULL, " ");
        config->voyages[n].destination = strtok(NULL, " ");
        config->voyages[n].reward = atoi(strtok(NULL, " "));
        config->numVoyages++;

        line = readLine(fd);
    }

    close(fd);
    return 0;
}

/************************************************
 *
 * @Name: loadIthacaConfig
 * @Def: Reads ithaca.dat and voyages.dat. The format is assumed correct (p.18).
 * @Arg: In: configPath = path of ithaca.dat
 *       In: voyagesPath = path of voyages.dat
 *       Out: config = the loaded configuration
 * @Ret: 0 on success, -1 on error (nothing is left allocated).
 *
 ************************************************/
int loadIthacaConfig(char *configPath, char *voyagesPath, IthacaConfig *config) {

    int fd = 0;

    config->numVoyages = 0;
    config->voyages = NULL;

    fd = open(configPath, O_RDONLY);
    if (-1 == fd) {
        printError("Error: could not open the Ithaca config file.\n");
        return -1;
    }
    config->name = readLine(fd);
    config->folder = readLine(fd);
    config->ip = readLine(fd);
    strtok(config->ip, " ");
    config->port = atoi(strtok(NULL, " "));
    close(fd);

    if (-1 == loadVoyages(voyagesPath, config)) {
        freeIthacaConfig(config);
        return -1;
    }
    return 0;
}

/************************************************
 *
 * @Name: freeIthacaConfig
 * @Def: Frees everything loadIthacaConfig allocated.
 * @Arg: In/Out: config = configuration to free
 * @Ret: -
 *
 ************************************************/
void freeIthacaConfig(IthacaConfig *config) {

    int i = 0;

    free(config->name);
    free(config->folder);
    free(config->ip);
    for (i = 0; i < config->numVoyages; i++) {
        free(config->voyages[i].object);
    }
    free(config->voyages);
}