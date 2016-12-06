/*      (C)2000 FEUP  */

#include "Util.h"

int ftpTransfer(char argv[]){

	int	sockfd, datafd, bytes;
	struct urlInfo info;
	if(parseAddress(&info, argv) != 0){
		printf("wrong syntax\n");
		exit(2);
	}

	if((sockfd = connectServer(info.hostName, FTP_PORT)) == -1){
		exit(3);
	}

	char response[512];
	bytes = read(sockfd, response, sizeof(response));
	write(0, response, bytes);

	if(login(sockfd, info.user, info.pass) != 0){
		exit(4);
	}

	datafd = pasv(sockfd, info.hostName);

	char file[512];
	sprintf(file, "RETR %s\n", info.path);
	write(sockfd, file, strlen(file));
	bytes = read(sockfd, response, sizeof(response));
	write(0, response, bytes);

	//transfer(datafd, info.fileName);

	close(sockfd);
	return 0;
}

int main(int argc, char** argv){

	if(argc != 2){
		printf("Usage %s ftp://[<user>:<pass>@]<host>/<url-path>\n", argv[0]);
		exit(1);
	}

	return ftpTransfer(argv[1]);
}
