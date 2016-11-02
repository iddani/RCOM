#include "utilities.h"

int checkFile(char *fileName){
	int fd = open(fileName, O_RDONLY);
	if(fd < 0){
		printf("Error opening file\n");
		return -1;
	} else{
		close(fd);
		return 0;
	}

}

int checkBaudRate(int input){
	switch (input){
		case 110:
			return 0xB110;
		case 300:
			return 0xB300;
		case 600:
			return 0xB600;
		case 1200:
			return 0xB1200;
		case 2400:
			return 0xB2400;
		case 4800:
			return 0xB4800;
		case 9600:
			return 0xB9600;
		case 14400:
			return 0xB14400;
		case 19200:
			return 0xB19200;
		case 28800:
			return 0xB28800;
		case 38400:
			return 0xB38400;
		case 56000:
			return 0xB56000;
		case 57600:
			return 0xB57600;
		case 115200:
			return 0xB115200;
		case 128000:
			return 0xB128000;
		case 153600:
			return 0xB153600;
		case 230400:
			return 0xB230400;
		case 256000:
			return 0xB256000;
		case 460800:
			return 0xB460800;
		case 9216600:
			return 0xB9216600;
		default:
			return -1;
	}
}
