#include "socket.h"

static int fillAddress(char *psIp, int nPort, struct sockaddr_in *pstAddress) {

    memset(pstAddress, 0, sizeof(struct sockaddr_in));
    pstAddress->sin_family = AF_INET;
    pstAddress->sin_port = htons(nPort);
    if (1 != inet_pton(AF_INET, psIp, &pstAddress->sin_addr)) {
        return -1;
    }
    return 0;
}

int openClientConn(char *psIp, int nPort) {

    struct sockaddr_in stAddress;
    int nFd = 0;

    if (-1 == fillAddress(psIp, nPort, &stAddress)) {
        return -1;
    }
    nFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == nFd) {
        return -1;
    }
    if (-1 == connect(nFd, (struct sockaddr *) &stAddress, sizeof(stAddress))) {
        close(nFd);
        return -1;
    }
    return nFd;
}

int openListenConn(char *psIp, int nPort) {

    struct sockaddr_in stAddress;
    int nFd = 0;
    int nReuse = 1;

    if (-1 == fillAddress(psIp, nPort, &stAddress)) {
        return -1;
    }
    nFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == nFd) {
        return -1;
    }
    if (-1 == setsockopt(nFd, SOL_SOCKET, SO_REUSEADDR, &nReuse, sizeof(nReuse)) ||
        -1 == bind(nFd, (struct sockaddr *) &stAddress, sizeof(stAddress)) ||
        -1 == listen(nFd, LISTEN_BACKLOG)) {
        close(nFd);
        return -1;
    }
    return nFd;
}