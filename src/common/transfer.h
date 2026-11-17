#ifndef __TRANSFER_H__
#define __TRANSFER_H__

#include <errno.h>        
#include <sys/stat.h>     
#include <sys/wait.h>     // waitpid

#include "frame.h"        
#include "io.h"         

#define MD5_LENGTH 32       
#define HEADER_FIELDS 4     // Kind&FileName&FileSize&MD5SUM

int sendFile(int nFd, char *psKind, char *psPath, char *psName, int nFlags);
char *receiveFile(int nFd, Frame *pstHeader, char *psKind, char *psFolder);
char *fileNameOf(char *psPath);

#endif