#ifndef __IO_H__
#define __IO_H__

/************************************************
 *
 * @File: io.h
 * @Purpose: Input/output helpers shared by Odysseus, Ithaca and Island.
 *           Only read() and write() are used, as the statement requires.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Prototypes
int printString(char *psText);
int printError(char *psText);
char *readLine(int nFd);
int readInputLine(int nFd, char **ppsLine);

#endif