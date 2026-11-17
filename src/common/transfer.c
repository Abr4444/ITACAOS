/************************************************
 * @File: transfer.c
 * @Purpose: Sends and receives whole files (the voyage images) with the
 *           HEADER, READY, DATA and ACK frames, checked with md5sum.
 * @Date: 17/11/2026
 ************************************************/

#define _GNU_SOURCE
#include "transfer.h"

/* @Name: readPipe
 * @Def: Reads nWanted bytes from a pipe, looping until it has them all
 *       or the pipe closes.
 * @Arg: In: nFd = read end of the pipe
 *       Out: pBuffer = the bytes read
 *       In: nWanted = number of bytes to read
 * @Ret: The number of bytes read (fewer than nWanted if the pipe closed). */
static int readPipe(int nFd, char *pBuffer, int nWanted) {
    int nTotal = 0;
    int nRead = 1;

    while (nWanted > nTotal && 0 < nRead) {
        nRead = read(nFd, &pBuffer[nTotal], nWanted - nTotal);
        if (0 < nRead)
            nTotal += nRead;
    }
    return nTotal;
}

/* @Name: getMd5
 * @Def: Runs md5sum on a file in a child process and reads the hash
 *       through a pipe.
 * @Arg: In: psPath = file to hash
 *       Out: sMd5 = the 32-character hash (MD5_LENGTH + 1 bytes)
 * @Ret: 0 on success, -1 on error. */
static int getMd5(char *psPath, char *sMd5) {
    int anFd[2];
    int nPid = 0;
    int nRead = 0;

    if (-1 == pipe(anFd))
        return -1;

    nPid = fork();
    if (-1 == nPid) {
        close(anFd[0]);
        close(anFd[1]);
        return -1;
    }

    if (0 == nPid) {
        close(anFd[0]);
        dup2(anFd[1], STDOUT_FILENO);
        close(anFd[1]);
        execlp("md5sum", "md5sum", psPath, NULL);
        _exit(1);
    }
    else {
        close(anFd[1]);
        nRead = readPipe(anFd[0], sMd5, MD5_LENGTH);
        close(anFd[0]);
        waitpid(nPid, NULL, 0);
        sMd5[nRead] = '\0';

        if (nRead != MD5_LENGTH)
            return -1;
        return 0;
    }
    return -1;
}

/* @Name: fileNameOf
 * @Def: Gives the file name of a path: what comes after the last '/'
 * @Arg: In: psPath = the path
 * @Ret: A pointer into psPath */
char* fileNameOf(char* psPath) {
    char *psName = psPath;
    int i = 0;

    for (i = 0; '\0' != psPath[i]; i++) {
        if ('/' == psPath[i])
            psName = &psPath[i + 1];
    }
    return psName;
}

/* @Name: sendFile
 * @Def: Sends a whole file: HEADER, waits READY, DATA frames
 * @Arg: In: nFd = socket of the connection
 *       In: psKind = "OBJECT", "DELIVERY", "SIGNED" or "CLAIM"
 *       In: psPath = the file to send
 *       In: psName = file name written in the header
 *       In: nFlags = FLAGS_OK (Ithaca, Island) or FLAGS_REQUEST (Odysseus)
 * @Ret: 0 if the receiver answered ACK OK, -1 otherwise. */
int sendFile(int nFd, char *psKind, char *psPath, char *psName, int nFlags) {
    char acData[FRAME_DATA_SIZE];
    char sMd5[MD5_LENGTH + 1];
    char *psHeader = NULL;
    Frame stFrame;
    int nFileFd = 0;
    int nSize = 0;
    int nRead = 1;
    int nError = 0;

    nFileFd = open(psPath, O_RDONLY);
    if (-1 == nFileFd)
        return -1;

    nSize = lseek(nFileFd, 0, SEEK_END);
    lseek(nFileFd, 0, SEEK_SET);
    // Kind&FileName&FileSize&MD5  ex. OBJECT&image.png&size&md5
    if (0 >= nSize || -1 == getMd5(psPath, sMd5) ||
        -1 == asprintf(&psHeader, "%s&%s&%d&%s", psKind, psName, nSize, sMd5)) {
        close(nFileFd);
        return -1;
    }

    if (-1 == sendFrame(nFd, TYPE_FILE_HEADER, nFlags, psHeader))
        nError = 1;
    free(psHeader);

    // wait for READY 0x80
    if (0 == nError && (-1 == receiveFrame(nFd, &stFrame) || TYPE_FILE_READY != stFrame.nType || FLAGS_OK != stFrame.nFlags))
        nError = 1;

    while (0 == nError && 0 < nRead) {
        nRead = read(nFileFd, acData, FRAME_DATA_SIZE);
        if (-1 == nRead)
            nError = 1;
        if (0 < nRead && -1 == sendBinaryFrame(nFd, TYPE_FILE_DATA, nFlags | FLAG_BINARY, acData, nRead))
            nError = 1;
    }

    if (0 == nError && (-1 == receiveFrame(nFd, &stFrame) || TYPE_FILE_ACK != stFrame.nType || FLAGS_OK != stFrame.nFlags))
        nError = 1;

    close(nFileFd);
    if (1 == nError)
        return -1;
    return 0;
}

/* @Name: receiveFile
 * @Def: Receives a whole file checks it, answers READY, saves the DATA frames into
 *       psFolder/<FileName>, checks size and md5 and answers ACK.
 * @Arg: In: nFd = socket of the connection
 *       In/Out: pstHeader = the HEADER frame (its DATA gets split)
 *       In: psKind = kind expected: "OBJECT", "DELIVERY", "SIGNED" or "CLAIM"
 *       In: psFolder = folder where the file is saved
 * @Ret: The path of the saved file (the caller frees it), or NULL on error */
char *receiveFile(int nFd, Frame *pstHeader, char *psKind, char *psFolder) {
    char *apsFields[HEADER_FIELDS];
    char sActual[MD5_LENGTH + 1];
    char *psPath = NULL;
    char *psReason = NULL;
    Frame stFrame;
    int nFileFd = 0;
    int nSize = 0;
    int nReceived = 0;
    int i = 0;

    // im sorry for all these ifs, but they are checks to ensure the data is correct, explained here:
        //   - it's not a HEADER frame (0x20)
        //   - our folder doesn't exist and can't be created
        //   - it doesn't have the 4 fields Kind&FileName&FileSize&MD5
        //   - the kind isn't the one we expect 
        //   - the name has a '/' (it would be saved outside our folder)
        //   - the size isn't > 0 or the md5 isn't 32 characters
    if (TYPE_FILE_HEADER != pstHeader->nType || (-1 == mkdir(psFolder, 0755) && EEXIST != errno) ||
        HEADER_FIELDS != splitFields(pstHeader->sData, apsFields, HEADER_FIELDS) ||
        0 != strcmp(psKind, apsFields[0]) || fileNameOf(apsFields[1]) != apsFields[1] ||
        0 >= atoi(apsFields[2]) || MD5_LENGTH != (int) strlen(apsFields[3]) ||
        -1 == asprintf(&psPath, "%s/%s", psFolder, apsFields[1])) {
            sendFrame(nFd, TYPE_FILE_READY, FLAGS_FAIL, "INVALID_HEADER");
            return NULL;
    }
    nFileFd = open(psPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (-1 == nFileFd) {
        free(psPath);
        sendFrame(nFd, TYPE_FILE_READY, FLAGS_FAIL, "INVALID_HEADER");
        return NULL;
    }
    sendFrame(nFd, TYPE_FILE_READY, FLAGS_OK, "READY");

    nSize = atoi(apsFields[2]);
    for (i = 0; i < (nSize + FRAME_DATA_SIZE - 1) / FRAME_DATA_SIZE && 0 == receiveFrame(nFd, &stFrame); i++) {
        if (TYPE_FILE_DATA == stFrame.nType && stFrame.nLength == write(nFileFd, stFrame.sData, stFrame.nLength))
            nReceived += stFrame.nLength;
    }
    close(nFileFd);

    if (nSize != nReceived)
        psReason = "SIZE_ERROR";
    else if (-1 == getMd5(psPath, sActual) || 0 != strcmp(apsFields[3], sActual))
        psReason = "MD5_ERROR";

    if (NULL == psReason) {
        sendFrame(nFd, TYPE_FILE_ACK, FLAGS_OK, "OK");
        return psPath;
    }

    sendFrame(nFd, TYPE_FILE_ACK, FLAGS_FAIL, psReason);
    unlink(psPath);
    free(psPath);
    return NULL;
}