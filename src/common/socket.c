/************************************************
 * @File: socket.c
 * @Purpose: Opens TCP connections: listening sockets and client connections.
 * @Date: 25/10/2026
 ************************************************/

#include "socket.h"

/* @Name: fillAddress
 * @Def: Fills an IPv4 address from an IP and a port.
 * @Arg: In: psIp = IP as text
 *       In: nPort = port
 *       Out: pstAddress = the address
 * @Ret: 0 on success, -1 if the IP is not valid */
static int fillAddress(char *psIp, int nPort, struct sockaddr_in *pstAddress) {

    memset(pstAddress, 0, sizeof(struct sockaddr_in));
    pstAddress->sin_family = AF_INET;
    pstAddress->sin_port = htons(nPort);
    if (1 != inet_pton(AF_INET, psIp, &pstAddress->sin_addr)) {
        return -1;
    }
    return 0;
}

/* @Name: openClientConn
 * @Def: Connects to a server.
 * @Arg: In: psIp = IP of the server
 *       In: nPort = port of the server
 * @Ret: The socket, or -1 on error */
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

/* @Name: openListenConn
 * @Def: Opens a listening socket on an IP and port.
 * @Arg: In: psIp = IP to listen on
 *       In: nPort = port to listen on
 * @Ret: The socket, or -1 on error */
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