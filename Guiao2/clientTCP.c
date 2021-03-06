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

	char response[BUFFER_SIZE];
	bytes = read(sockfd, response, sizeof(response));
	write(0, response, bytes);

	if(login(sockfd, info.user, info.pass) != 0){
		exit(4);
	}

	datafd = pasv(sockfd, info.hostName);

	if(retr(sockfd, info.path) != 0){
		exit(5);
	}

	transfer(datafd, info.fileName);

	quit(sockfd);
	return 0;
}

int main(int argc, char** argv){

	if(argc != 2){
		printf("Usage %s ftp://[<user>:<pass>@]<host>/<url-path>\n", argv[0]);
		exit(1);
	}

	return ftpTransfer(argv[1]);
}
