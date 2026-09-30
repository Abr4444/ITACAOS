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
    char cChar = ' ';
    int nLength = 0;

    while (1 == read(nFd, &cChar, 1) && '\n' != cChar) {
        psLine = realloc(psLine, nLength + 2);
        if (NULL == psLine) {
            printError("Error: not enough memory to read a line.\n");
            return NULL;
        }
        psLine[nLength] = cChar;
        nLength++;
    }

    if (NULL != psLine) {
        psLine[nLength] = '\0';
    }
    return psLine;
}