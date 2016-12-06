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
	if(last == NULL){
		strcpy(info->fileName, info->path);
	} else {
		memcpy(info->fileName, &last[1], strlen(last));
	}

	// char *path = malloc(512);
	// strncpy(path, info->path, strlen(info->path)-strlen(last));
	//free(path);
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

int transfer(int datafd, char fileName[]){
	FILE *f = fopen(fileName, "w");

	int bytes;
	char response[1024];
	while ((bytes=read(datafd, response, sizeof(char)*1024)) > 0) {
		fwrite(response, bytes, 1, f);
	}

	close(datafd);
	fclose(f);
	return 0;
}

int login(int sockfd, char user[], char pass[]){
	char send[64], response[512];
	int bytes;

	sprintf(send, "USER %s\n", user);
	write(sockfd, send, strlen(send));
	read(sockfd, response, sizeof(response));

	sprintf(send, "PASS %s\n", pass);
	write(sockfd, send, strlen(send));
	bytes = read(sockfd, response, sizeof(response));

	write(0, response, bytes);

	char *status = strtok(response, " ");
	status = strtok(NULL, ".");
	if(strcasecmp(status, "Login successful") != 0){
		return -1;
	}
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

int retr(int sockfd, char path[]){
	char file[512], response[512];
	int bytes;
	sprintf(file, "RETR %s\n", path);
	write(sockfd, file, strlen(file));
	bytes = read(sockfd, response, sizeof(response));
	write(0, response, bytes);

	char *status = strtok(response, " ");
	status = strtok(NULL, ".");
	if(strcasecmp(status, "Failed to open file") == 0){
		return -1;
	}
	return 0;
}

int quit(int sockfd){

	char quit[5] = "QUIT";
	write(sockfd, quit, strlen(quit));
	close(sockfd);
	return 0;
}
