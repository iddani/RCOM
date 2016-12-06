#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define SERVER_PORT 21

struct urlInfo {
	char user[64];
	char pass[64];
	char hostName[128];
	char path[256];
};

int parseAddress(struct urlInfo *info, char argv[]);
