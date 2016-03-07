#ifndef MAIN_H
#define MAIN_H

#endif // MAIN_H
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>

#define MAX_MESSAGE_SIZE 20000
#define EXAMPLE_PORT 16000
#define EXAMPLE_ADDR "239.0.0.1"
#define RECV_PORT 16001
#define MESSAGE_LEN 75

#define SEND 1
#define RECV 2

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff /* should be in <netinet/in.h> */
#endif

/* msockcreate -- Create socket from which to read.
   return socket descriptor if ok, -1 if not ok.  */
int msockcreate(int type, char *address, int port);

/* msockdestroy -- Destroy socket by closing.
   return socket descriptor if ok, -1 if not ok.  */
int msockdestroy(int sock);

/* msend -- Send multicast essage to given address.
   return number of bytes sent, -1 if error. */
int msend(int sock, char *message, int len);

/* mrecv -- Receive message on given mcast address. Will block.
   return bytes received, -1 if error. */
int mrecv(int sock, char *message, int max_len);
