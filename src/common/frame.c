/************************************************
 * @File: frame.c
 * @Purpose: Fixed 256-byte frames of the protocol: send, receive, check and NACK.
 * @Date: 25/10/2026
 ************************************************/

#include "frame.h"

/* @Name: sendBinaryFrame
 * @Def: Serializes a frame whose DATA may contain any byte (also '\0')
 *       and sends it with one write of 256 bytes.
 * @Arg: In: nFd = socket
 *       In: nType, nFlags = header of the frame
 *       In: pData = bytes of the frame, or NULL if nLength is 0
 *       In: nLength = number of valid bytes in pData
 * @Ret: 0 on success, -1 on error */
int sendBinaryFrame(int nFd, int nType, int nFlags, char* pData, int nLength){
    unsigned char aucFrame[FRAME_SIZE];

    if(0 > nLength || FRAME_DATA_SIZE < nLength)
        return -1;

    memset(aucFrame, 0, FRAME_SIZE);
    aucFrame[0] = (unsigned char) nType;
    aucFrame[1] = (unsigned char) nFlags;
    aucFrame[2] = (unsigned char) nLength;

    if(0 < nLength)
        memcpy(&aucFrame[FRAME_HEADER_SIZE], pData, nLength);

    if(FRAME_SIZE != write(nFd, aucFrame, FRAME_SIZE))
        return -1;

    return 0;
}

/* @Name: sendFrame
 * @Def: Sends a frame whose data is text
 * @Arg: In: nFd = socket
 *       In: nType, nFlags = header of the frame
 *       In: psData = text of the frame, or NULL
 * @Ret: 0 on success, -1 on error */
int sendFrame(int nFd, int nType, int nFlags, char* psData){
    int nLength = 0;

    if(NULL != psData)
        nLength = strlen(psData);

    return sendBinaryFrame(nFd, nType, nFlags, psData, nLength);
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
           (TYPE_FILE_HEADER <= nType && TYPE_FILE_ACK >= nType) ||
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
 *       field ends with '\0' and apsFields points into it. No strtok,
 *       so several threads can split frames at the same time.
 * @Arg: In/Out: psData = DATA of the frame
 *       Out: apsFields = the fields found
 *       In: nMax = number of fields expected
 * @Ret: The number of fields found (at most nMax).
 ************************************************/
int splitFields(char *psData, char *apsFields[], int nMax) {

    int nCount = 0;
    int i = 0;

    if ('\0' == psData[0] || 0 >= nMax)
        return 0;
    apsFields[nCount] = psData;
    nCount++;
    for (i = 0; '\0' != psData[i] && nCount < nMax; i++) {
        if ('&' == psData[i]) {
            psData[i] = '\0';
            apsFields[nCount] = &psData[i + 1];
            nCount++;
        }
    }
    return nCount;
}