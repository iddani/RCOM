#include "Util.h"

int parseAddress(struct urlInfo *info,char argv[]){

	char *newUrl = strtok(argv, "[");
	newUrl = strtok(NULL, ":");
	if(newUrl == NULL) return -1;
	sprintf(info->user, "%s",newUrl);

	char *pass = strtok(NULL, "@");
	if(pass == NULL) return -1;
	sprintf(info->pass, "%s", pass);

	newUrl = strtok(NULL, "]");
	char address[64];
	sprintf(address, "%s", newUrl);
	char *host = strtok(newUrl, "/");
	if(host == NULL) return -1;

	sprintf(info->hostName, "%s", host);
	if(strtok(NULL, "/") == NULL) return -1;
	memcpy(info->path, &address[strlen(host)+1], strlen(address)-strlen(host)+1);

	char *last = strrchr(info->path, '/');
	memcpy(info->fileName, &last[1], strlen(last));

	return 0;
}

int connectServer(char hostName[], int serverPort){
	int sockfd, status;
	struct addrinfo hints, *p, *servinfo;
	char port[16];

	bzero((char *)&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;

	sprintf(port, "%d", serverPort);
	if ((status = getaddrinfo(hostName, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}
	freeaddrinfo(servinfo);
	return sockfd;
}

int calculateDataPort(char address[]){
	int port;
	char *str;
	strtok(address, ",");
	strtok(NULL, ",");
	strtok(NULL, ",");
	strtok(NULL, ",");
	str = strtok(NULL, ",");
	port = strtol(str, NULL, 10) * 256;
	str = strtok(NULL, ",");
	port += strtol(str, NULL, 10);

	return port;
}

int transfer(){
	return 0;
}

int login(int sockfd, char user[], char pass[]){
	char send[64], response[512];
	int bytes;

	sprintf(send, "USER %s\n", user);
	bytes = write(sockfd, send, strlen(send));
	bytes = read(sockfd, response, sizeof(response));

	sprintf(send, "PASS %s\n", pass);
	bytes = write(sockfd, send, strlen(send));
	bytes = read(sockfd, response, sizeof(response));

	write(0, response, bytes);
	return 0;
}

int pasv(int sockfd, char hostName[]){
	int bytes, dataPort, datafd;
	char pasv[8] = "PASV\n", response[512], *dataIP;
	bytes = write(sockfd, pasv, strlen(pasv));
	bytes = read(sockfd, response, sizeof(response));
	if(bytes < 0){
		printf("Error reading response from PASV\n");
		return -1;
	}

	strtok(response, " ");
	dataIP = strtok(NULL, "Entering Passive Mode(");
	dataIP = strtok(dataIP, ")");
	dataPort = calculateDataPort(dataIP);

	if((datafd = connectServer(hostName, dataPort)) == -1){
		return -1;
	}
	return datafd;
}
