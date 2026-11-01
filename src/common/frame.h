#ifndef __FRAME_H__
#define __FRAME_H__

/************************************************
 * @File: frame.h
 * @Purpose: Fixed 256-byte frames of the protocol: send, receive, check and NACK.
 * @Date: 25/10/2026
 ************************************************/

#include <string.h>
#include <unistd.h>

#define FRAME_SIZE 256
#define FRAME_HEADER_SIZE 3
#define FRAME_DATA_SIZE 253

//frame types Odysseus <-> Ithaca
#define TYPE_CONNECT_ITHACA 0x01
#define TYPE_LIST_VOYAGES 0x02
#define TYPE_ACCEPT 0x03
#define TYPE_CLAIM 0x04
#define TYPE_DISCONNECT_ITHACA 0x05

//frame types Odysseus <-> Island
#define TYPE_ARRIVAL 0x10
#define TYPE_LIST_MARKET 0x11
#define TYPE_BUY 0x12
#define TYPE_SELL 0x13
#define TYPE_BUY_MAP 0x14
#define MAP_PRICE 50

#define TYPE_DELIVER 0x15
#define TYPE_DEPARTURE 0x16

//any process; for frames with invalid format
#define TYPE_NACK 0x7F

//reasons sent in a NACK frame
#define NACK_INVALID_TYPE "INVALID_TYPE"
#define NACK_INVALID_FLAGS "INVALID_FLAGS"
#define NACK_INVALID_LENGTH "INVALID_LENGTH"
#define NACK_INVALID_DATA "INVALID_DATA"

//flag bits
#define FLAG_RESPONSE 0x80
#define FLAG_ERROR 0x40
#define FLAG_BINARY 0x20
#define FLAGS_RESERVED 0x1F
#define FLAGS_REQUEST 0x00
#define FLAGS_OK (FLAG_RESPONSE)
#define FLAGS_FAIL (FLAG_RESPONSE | FLAG_ERROR)


typedef struct{
    int nType;
    int nFlags;
    int nLength;
    char sData[FRAME_DATA_SIZE + 1];
} Frame;

int sendFrame(int nFd, int nType, int nFlags, char *psData);
int receiveFrame(int nFd, Frame *pstFrame);
char *checkFrame(Frame *pstFrame);
int sendNack(int nFd, char *psReason);

int splitFields(char *psData, char *apsFields[], int nMax);

#endif