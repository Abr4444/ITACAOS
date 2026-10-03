/************************************************
 *
 * @File: io.c
 * @Purpose: Input/output helpers shared by Odysseus, Ithaca and Island.
 *           Only read() and write() are used, as the statement requires.
 * @Author: Stefan Radulescu, Albert Baiget Rodulfo
 * @Date: 28/09/2026
 *
 ************************************************/

#include "io.h"

/************************************************
 *
 * @Name: printString
 * @Def: Writes a string to the screen (stdout).
 * @Arg: In: psText = string to write
 * @Ret: The number of bytes written, or -1 on error.
 *
 ************************************************/
int printString(char *psText) {

    return write(STDOUT_FILENO, psText, strlen(psText));
}

/************************************************
 *
 * @Name: printError
 * @Def: Writes an error message to stderr.
 * @Arg: In: psText = message to write
 * @Ret: The number of bytes written, or -1 on error.
 *
 ************************************************/
int printError(char *psText) {

    return write(STDERR_FILENO, psText, strlen(psText));
}

/************************************************
 *
 * @Name: readLine
 * @Def: Reads one line from a file, without the '\n'.
 * @Arg: In: nFd = file descriptor to read from
 * @Ret: A malloc'd copy of the line that the caller must free,
 *       or NULL at end of file, on an empty line or if memory runs out.
 *
 ************************************************/
char *readLine(int nFd) {

    char *psLine = NULL;
    char *psMore = NULL;
    char cChar = ' ';
    int nLength = 0;

    while (1 == read(nFd, &cChar, 1) && '\n' != cChar) {
        psMore = realloc(psLine, nLength + 2);
        if (NULL == psMore) {
            printError("Error: not enough memory to read a line.\n");
            free(psLine);
            return NULL;
        }
        psLine = psMore;
        psLine[nLength] = cChar;
        nLength++;
    }

    if (NULL != psLine) {
        psLine[nLength] = '\0';
    }
    return psLine;
}


/************************************************
 *
 * @Name: readInputLine
 * @Def: Reads one line typed in the terminal, without the '\n'.
 *       Unlike readLine, an empty line is a valid (empty) string.
 * @Arg: In: nFd = file descriptor to read from
 *       Out: ppsLine = malloc'd line that the caller must free
 * @Ret: 0 on success, -1 on end of input, error (CTRL+C closes stdin)
 *       or lack of memory (nothing is left allocated).
 *
 ************************************************/
int readInputLine(int nFd, char **ppsLine) {
 
    char *psLine = NULL;
    char *psMore = NULL;
    char cChar = ' ';
    int nLength = 0;
    int nRead = 0;
 
    psLine = malloc(1);
    if (NULL == psLine) {
        printError("Error: not enough memory to read a line.\n");
        return -1;
    }
    nRead = read(nFd, &cChar, 1);
    while (1 == nRead && '\n' != cChar) {
        psMore = realloc(psLine, nLength + 2);
        if (NULL == psMore) {
            printError("Error: not enough memory to read a line.\n");
            free(psLine);
            return -1;
        }
        psLine = psMore;
        psLine[nLength] = cChar;
        nLength++;
        nRead = read(nFd, &cChar, 1);
    }
 
    if (1 != nRead) {
        free(psLine);
        return -1;
    }
    psLine[nLength] = '\0';
    *ppsLine = psLine;
    return 0;
}