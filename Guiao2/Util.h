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

#define FTP_PORT 21

struct urlInfo {
	char user[64];
	char pass[64];
	char hostName[128];
	char path[256];
	char fileName[64];
};

int parseAddress(struct urlInfo *info, char argv[]);	/*retrieves the information from the url*/
int connectServer(char hostName[], int serverPort);		/*creates a connection to the hostName given*/
int calculateDataPort(char address[]);					/*calculates a port number given an IP address */
int login(int sockfd, char user[], char pass[]);		/*sends login data to ftp server*/
int pasv(int sockfd, char hostName[]);					/*sends pasv and opens a connection with given port number*/
int retr(int sockfd, char path[]);						/*sends retr command and analyses answer*/
int transfer(int datafd, char fileName[]);				/*receives file through second port*/
int quit();												/*closes ftp connection*/
