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

	return 0;
}
