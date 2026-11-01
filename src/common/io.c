/************************************************
 * @File: io.c
 * @Purpose: Input/output helpers shared by Odysseus, Ithaca and Island.
 *           Only read() and write() are used, as the statement requires.
 * @Date: 28/09/2026
 ************************************************/

#include "io.h"

/************************************************
 * @Name: printString
 * @Def: Writes a string to the screen (stdout).
 * @Arg: In: psText = string to write
 * @Ret: The number of bytes written, or -1 on error.
 ************************************************/
int printString(char *psText) {

    return write(STDOUT_FILENO, psText, strlen(psText));
}

/************************************************
 * @Name: printError
 * @Def: Writes an error message to stderr.
 * @Arg: In: psText = message to write
 * @Ret: The number of bytes written, or -1 on error.
 ************************************************/
int printError(char *psText) {

    return write(STDERR_FILENO, psText, strlen(psText));
}

/************************************************
 * @Name: readLine
 * @Def: Reads one line from a file, without the '\n'.
 *       An empty line is a valid line: it returns an empty string.
 * @Arg: In: nFd = file descriptor to read from
 * @Ret: A malloc'd copy of the line that the caller must free, or NULL
 *       at end of file with nothing read, on a read error (a closed
 *       stdin after CTRL+C) or if memory runs out.
 ************************************************/
char *readLine(int nFd) {

    char *psLine = NULL;
    char *psMore = NULL;
    char cChar = ' ';
    int nLength = 0;
    int nRead = 0;

    psLine = malloc(1);
    if (NULL == psLine) {
        printError("Error: not enough memory to read a line.\n");
        return NULL;
    }

    nRead = read(nFd, &cChar, 1);
    while (1 == nRead && '\n' != cChar) {
        if ('\r' != cChar) {   // ignore Windows line endings (CRLF)
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
        nRead = read(nFd, &cChar, 1);
    }

    // nRead is 1 if the line ended with '\n', 0 at end of file, -1 on error.
    // A last line without '\n' is still a line, but nothing at all is not.
    if (-1 == nRead || (0 == nRead && 0 == nLength)) {
        free(psLine);
        return NULL;
    }
    psLine[nLength] = '\0';
    return psLine;
}


/************************************************
 * @Name: copyText
 * @Def: Makes a malloc'd copy of a string.
 * @Arg: In: psText = string to copy
 * @Ret: The copy, that the caller must free, or NULL if memory runs out.
 ************************************************/
char *copyText(char *psText) {

    char *psCopy = malloc(strlen(psText) + 1);

    if (NULL != psCopy)
        strcpy(psCopy, psText);
    return psCopy;
}