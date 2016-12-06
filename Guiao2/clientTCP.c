/*      (C)2000 FEUP  */

#include "Util.h"

int login(int sockfd, char user[], char pass[]){

	return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv){

	int	sockfd;
	// struct	sockaddr_in server_addr;
	// int	bytes;
	struct urlInfo info;

	if(argc != 2){
		printf("Usage %s ftp://[<user>:<pass>@]<host>/<url-path>\n", argv[0]);
		exit(1);
	}
	// char name[200] = "ftp://[up23123:blablalbalbla@]www.google.com/wevvdvc.txt";
	if(parseAddress(&info, argv[1]) != 0){
		printf("wrong syntax\n");
		exit(1);
	}

	struct addrinfo *p, *servinfo;
	printf("%s\n", info.hostName);
	int status;
	struct addrinfo hints;
	bzero((char *)&hints, sizeof(hints));
	hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	char port[16];
	sprintf(port, "%d", SERVER_PORT);
	if ((status = getaddrinfo(info.hostName, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

	printf("%d\n", servinfo->ai_protocol);
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
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
		return 2;
	}
	char s[INET6_ADDRSTRLEN];
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connecting to %s\n", s);
	/*server address handling*/
	// bzero((char*)&server_addr,sizeof(server_addr));
	// server_addr.sin_family = AF_INET;
	// server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	// server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	 printf("%s\n", "alo");
	// if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
    // 		perror("socket()");
    //     	exit(0);
    // 	}
	// /*connect to the server*/
    // if(connect(sockfd, p->ai_addr, p->ai_addrlen) < 0){
    //     perror("connect()");
	// 	exit(0);
	// }
	char user[64];
	sprintf(user, "user %s", info.user);
	char pass[64];
	sprintf(pass, "pass %s", info.pass);
    // 	/*send a string to the server*/
	int bytes;
	bytes = write(sockfd, user, strlen(user));
	bytes = write(sockfd, pass, strlen(pass));
	//bytes = write(sockfd, pass, strlen(pass));
	printf("Bytes escritos %d\n", bytes);
	freeaddrinfo(servinfo);
	close(sockfd);
	exit(0);
}
