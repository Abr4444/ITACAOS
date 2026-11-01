#ifndef __SOCKET_H__
#define __SOCKET_H__

/************************************************
 * @File: socket.h
 * @Purpose: Opens TCP connections: listening sockets and client connections.
 * @Date: 25/10/2026
 ************************************************/

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_BACKLOG 10

int openClientConn(char *psIp, int nPort);
int openListenConn(char *psIp, int nPort);

#endif