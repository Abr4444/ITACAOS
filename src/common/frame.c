#include "frame.h"

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

int isKnownType(int nType) {

    return (TYPE_CONNECT_ITHACA <= nType && TYPE_DISCONNECT_ITHACA >= nType) ||
           (TYPE_ARRIVAL <= nType && TYPE_DEPARTURE >= nType) ||
           TYPE_NACK == nType;
}

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

int sendNack(int nFd, char *psReason){
    return sendFrame(nFd, TYPE_NACK, FLAGS_FAIL, psReason);
}

/************************************************
 *
 * @Name: splitFields
 * @Def: Splits the DATA of a frame on '&'. The data is modified: each
 *       field ends with '\0' and apsFields points into it.
 * @Arg: In/Out: psData = DATA of the frame
 *       Out: apsFields = the fields found
 *       In: nMax = number of fields expected
 * @Ret: The number of fields found (at most nMax).
 *
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