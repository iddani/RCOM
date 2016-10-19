/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include "utilities.h"

volatile int STOP=FALSE;
unsigned int timeouts = 0;
unsigned int again = FALSE;
unsigned int ns = 0x00;
struct termios oldtio,newtio;
struct applicationLayer appLayer;
struct linkLayer lLayer;

void alarm_handler(int signo)
{
    timeouts++;
    printf("timeouts: %d\n", timeouts);
    again = FALSE;
}

int readByte(char byte, int status){
	switch(status){
		case 0:
			if(byte == FLAG){
				return 1;
			} else return 0;
			break;

		case 1:
			if(byte != FLAG){
				return 2;
			} else return 1;

			break;
		case 2:
			if(byte == FLAG){
				return 3;
			} else return 2;
			break;
	}
}

char * byteStuffing(int fd){

	int it = 0;
	int tentativas = 0;
	char * data;

	while(res > 0){

		char byte;
		int res = read(fd, byte, 1);

		if(res <= 0){
			printf("Erro ao ler o ficheiro\n");
			break;
		}

		if(byte == FLAG){
			data[it] = 0x7D;
			data[++it] = 0x5E;
		} else if (byte == 0x7D){
			data[it] = 0x7D;
			data[++it] = 0x5D;
		} else{
			data[it] = byte;
		}
		it++;

	}

}

char * byteDestuffing(char * data, int size){

	char * destuffed;
	int i = 0;
	for (; i < size; ++i){

		if(data[i] == 0x7D && data[i+1] == 0x5E){
			destuffed[i] = 0x7E;
			continue;
		}
		if(data[i] == 0x7D && data[i+1] == 0x5D){
			destuffed[i] = 0x7D;
			continue;
		}
		destuffed[i] = data[i];

	}

	return destuffed;

}

ControlType analyse(char *msg){

	char address = msg[1], control = msg[2], bcc = msg[3];
	if(bcc != (address^control)){
		return -1;
	} else {
		return control;
	}
}

int readIFrames(){

}

int sendMessage(ControlType waitFor, char *msg){
	char buf;
	char ans[20];
	int ansLength = 0, status = 0;
	int res = 0;

	while((STOP == FALSE) && (timeouts < 3)){
		again = 1;
		res = write(appLayer.fd, msg, 6);
		printf("Wrote %d bytes to fd\n", res);

		alarm(3);
		
		while(again == TRUE && STOP == FALSE){
			if(res = read(appLayer.fd, &buf, 1)){
				status = readByte(buf, status);
				ans[ansLength] = buf;
				ansLength++;

				if(status == 3){
					ControlType type = analyse(msg);
					if(type == waitFor){
						STOP = TRUE;
						printf("Disconnect aknowledged\n");
						alarm(0);		//desativa flag
					} else {
						ansLength = 0;
					}
				}
					
			}
		};
	}
}

char *createControlPacket(int type){
		//fileInfo
	struct stat s;
	if (fstat(appLayer.fd, &s) == -1) {
  		printf("fstat(%d) returned error=%d.", appLayer.fd, errno);
	}

	char packet[10];
	packet[0] = FLAG;

	if(appLayer.transmission == TRANSMITTER){
		packet[1] = ADDRESS_SEND;
	} else{		/*Receiver*/
		packet[1] = ADDRESS_RECV;
	}
	ns = (ns^COUNTER_MODULE);
	packet[2] = ns;
	packet[3] = (packet[1]^packet[2]);	//BCC
	//Data
	packet[4] = type;		//start
	packet[5] = 0x00;		//type
	//packet[6] = 0x02;		//number of bytes
	//packet[7] = s.st_size;
	packet[9] = 0x01;		//name
	//packet[10] = 0x01;		//number of bytes
	//packet[11] = name;		//value

	return packet;
}

char *createSUFrame(ControlType type){

	char frame[6];
	frame[0] = FLAG;
	frame[4] = FLAG;
	frame[5] = '\0';

	switch(appLayer.transmission){
		case TRANSMITTER:
			frame[1] = ADDRESS_SEND;
			break;
		case RECEIVER:
			frame[1] = ADDRESS_RECV; 
			break;
	}

	switch(type){
		case SET:
			frame[2] = CTRL_SET;
			break;
		case DISC:
			frame[2] = CTRL_DISC;
			break;
		case UA:
			frame[2] = CTRL_UA;
			break;
		case RR:

			break;
		case REJ:

			break;
	}
	return frame;
}

int llopen(int gate, ConnectionMode connection){

    printf("New termios structure set\n");
	
	char msg = createSUFrame(SET);
	sendMessage(SET,msg);
	return fd;	
}


int llclose(int fd){
	char msg = createSUFrame(DISC);

	sendMessage(DISC, msg);

}


int main(int argc, char** argv)
{
    int fd, res;
    int sum = 0, speed = 0;

    
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort ConnectionMode Ba\n\tex: nserial /dev/ttyS1 TRANSMITTER||RECEIVER\n");
      exit(1);
    }

    ConnectionMode connection;
    int gate;
    if(strcmp("/dev/ttyS0", argv[1])==0){
    	gate = 0;
    } else if(strcmp("/dev/ttyS1", argv[1])==0){
    	gate = 1;
    } else{
   	printf("SerialPort not recognized\n\tuse: /dev/ttyS0 || /dev/ttyS1\n");
      	exit(1);
    }

    if(strcmp("TRANSMITTER", argv[2])==0){
    	appLayer.transmission = TRANSMITTER;
	} else if(strcmp("RECEIVER", argv[2])==0){
		appLayer.transmission = RECEIVER;
	} else{
		printf("ConnectionMode not recognized\n\tuse: TRANSMITTER||RECEIVER\n");
      		exit(1);
	}

    struct sigaction action;
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM,&action, NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGALRM handler\n");
        exit(1);
    }

    int fd = open(port, O_RDWR | O_NOCTTY );

    if (fd <0) {perror(port); exit(-1); }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  	*/
    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      	perror("tcgetattr");
      	exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */


  	/* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  	*/


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);

    }


    appLayer.fd = llopen(gate, connection);


	
/*
	int gif = open("pinguim.gif", O_RDONLY);
	createStartFrame(gif,"pinguim.gif" );

	char pic;
	char data[255];
	int it = 0;
*/

/*
	while(it < 255){
		read( gif, pic, 1);
		if(pic == FLAG){
			data[it] = 0x7D;
			data[++it] = 0x5E;
		} else if (pic == 0x7D){
			data[it] = 0x7D;
			data[++it] = 0x5D;
		} else{
			data[it] = pic;
		}
		it++;

		if(it == 255){
			createSendFrame();
			it =0;
		}
	}
*/

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
