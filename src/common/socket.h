#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_BACKLOG 10

int openClientConn(char *psIp, int nPort);
int openListenConn(char *psIp, int nPort);

#endif