/************************************************
 * @File: frame.c
 * @Purpose: Fixed 256-byte frames of the protocol: send, receive, check and NACK.
 * @Date: 25/10/2026
 ************************************************/

#include "frame.h"

/* @Name: sendFrame
 * @Def: Serializes a frame and sends it with one write of 256 bytes.
 * @Arg: In: nFd = socket
 *       In: nType, nFlags = header of the frame
 *       In: psData = text of the frame, or NULL
 * @Ret: 0 on success, -1 on error */
int sendFrame(int nFd, int nType, int nFlags, char* psData){
    unsigned char aucFrame[FRAME_SIZE];
    int nLength = 0;

    if(NULL != psData)
        nLength = strlen(psData);

    if(FRAME_DATA_SIZE < nLength)
        return -1;

    memset(aucFrame, 0, FRAME_SIZE);
    aucFrame[0] = (unsigned char) nType;
    aucFrame[1] = (unsigned char) nFlags;
    aucFrame[2] = (unsigned char) nLength;

    if(0 < nLength)
        memcpy(&aucFrame[FRAME_HEADER_SIZE], psData, nLength);

    if(FRAME_SIZE != write(nFd, aucFrame, FRAME_SIZE))
        return -1;

    return 0;
}

/* @Name: receiveFrame
 * @Def: Reads one whole 256-byte frame and decodes it.
 * @Arg: In: nFd = socket
 *       Out: pstFrame = the frame received
 * @Ret: 0 on success, -1 if the connection closed */
int receiveFrame(int nFd, Frame *pstFrame) {
    unsigned char aucFrame[FRAME_SIZE];
    int nTotal = 0, nRead = 0;

    while (FRAME_SIZE > nTotal) {
        nRead = read(nFd, &aucFrame[nTotal], FRAME_SIZE - nTotal);
        if (0 >= nRead) {
            return -1;
        }
        nTotal += nRead;
    }

    pstFrame->nType = aucFrame[0];
    pstFrame->nFlags = aucFrame[1];
    pstFrame->nLength = aucFrame[2];
    memset(pstFrame->sData, 0, FRAME_DATA_SIZE + 1);
    if (FRAME_DATA_SIZE >= pstFrame->nLength) {
        memcpy(pstFrame->sData, &aucFrame[FRAME_HEADER_SIZE], pstFrame->nLength);
    }
    return 0;
}

/* @Name: isKnownType
 * @Def: Tells if a TYPE exists in the protocol.
 * @Arg: In: nType = type to check
 * @Ret: 1 if it exists, 0 otherwise */
int isKnownType(int nType) {

    return (TYPE_CONNECT_ITHACA <= nType && TYPE_DISCONNECT_ITHACA >= nType) ||
           (TYPE_ARRIVAL <= nType && TYPE_DEPARTURE >= nType) ||
           TYPE_NACK == nType;
}

/* @Name: checkFrame
 * @Def: Checks the type, flags, length and data of a frame.
 * @Arg: In: pstFrame = frame to check
 * @Ret: The NACK reason, or NULL if the frame is valid */
char *checkFrame(Frame *pstFrame) {

    if(0 == isKnownType(pstFrame->nType))
        return NACK_INVALID_TYPE;

    if(0 != (pstFrame->nFlags & FLAGS_RESERVED))
        return NACK_INVALID_FLAGS;

    if(0 != (pstFrame->nFlags & FLAG_ERROR) && 0 == (pstFrame->nFlags & FLAG_RESPONSE))
        return NACK_INVALID_FLAGS;

    if(FRAME_DATA_SIZE < pstFrame->nLength)
        return NACK_INVALID_LENGTH;

    if(0 == (pstFrame->nFlags & FLAG_BINARY) && (int) strlen(pstFrame->sData) != pstFrame->nLength)
        return NACK_INVALID_DATA;

    return NULL;
}

/* @Name: sendNack
 * @Def: Sends a NACK (0x7F) with the reason.
 * @Arg: In: nFd = socket
 *       In: psReason = reason of the NACK
 * @Ret: 0 on success, -1 on error */
int sendNack(int nFd, char *psReason){
    return sendFrame(nFd, TYPE_NACK, FLAGS_FAIL, psReason);
}

/************************************************
 * @Name: splitFields
 * @Def: Splits the DATA of a frame on '&'. The data is modified: each
 *       field ends with '\0' and apsFields points into it.
 * @Arg: In/Out: psData = DATA of the frame
 *       Out: apsFields = the fields found
 *       In: nMax = number of fields expected
 * @Ret: The number of fields found (at most nMax).
 ************************************************/
int splitFields(char *psData, char *apsFields[], int nMax) {

    char *psField = NULL;
    int nCount = 0;

    psField = strtok(psData, "&");
    while (NULL != psField && nCount < nMax) {
        apsFields[nCount] = psField;
        nCount++;
        psField = strtok(NULL, "&");
    }
    return nCount;
}