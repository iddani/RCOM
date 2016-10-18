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

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define ADDRESS_SEND 0x03
#define ADDRESS_RECV 0x01
#define CONTROL 0x03
#define BCC 0x02
#define COUNTER	0x40


volatile int STOP=FALSE;
unsigned int timeouts = 0;
unsigned int again = FALSE;
unsigned int ns = 0x00;


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

int isUA(char *msg){
	if(msg[3] != (msg[2]^msg[1])){
		return -1;
	} else {
		if(msg[2] == CONTROL){
			return TRUE;
		} else {
			return 0;
		}

	}
}

char* createStartFrame(int fd){

	//fileInfo
	struct stat s;
	if (fstat(fd, &s) == -1) {
  		printf("fstat(%d) returned error=%d.", fd, errno);
	}

	char packet[20];
	packet[0] = FLAG;
	packet[1] = ADDRESS_SEND;
	ns = (ns^COUNTER);
	packet[2] = ns;
	packet[3] = (packet[1]^packet[2]);	//BCC
	//Data
	packet[4] = 0x02;		//start
	packet[5] = 0x00;		//type
	packet[6] = 0x02;		//number of bytes
	packet[7] = s.st_size;
	packet[9] = 0x01;		//name
	packet[10] = 0x01;
	packet[11] = 0x01;

printf("%x\n", s.st_size);
	printf("%x\n", packet[7]);
	printf("%x\n", packet[8]);

	return packet;
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    int sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

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

    struct sigaction action;
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM,&action, NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGALRM handler\n");
        exit(1);
    }



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

	char msg[6];
	msg[0] = FLAG;
	msg[1] = ADDRESS_SEND;
	msg[2] = CONTROL;
	msg[3] = ADDRESS_SEND^CONTROL;
	msg[4] = FLAG;
	msg[5] = '\0';

	char buf;
	char ans[20];
	int ansLength = 0, status = 0;

	//sprintf(msg, "%x%x%x%x%x\n", FLAG,ADDRESS_SEND, CONTROL, ADDRESS_SEND^CONTROL, FLAG	);
	for (int i = 0; i < 6; ++i)
	{
		printf("%x", msg[i]);
	}
	printf("\n");
	
	while((STOP == FALSE) && (timeouts < 3)){
		again = 1;
		res = write(fd, msg, 6);
		printf("Wrote %d bytes to fd\n", res);
		alarm(3);
		
		do{
			if(res = read(fd, &buf, 1)){
				status = readByte(buf, status);
				ans[ansLength] = buf;
				ansLength++;

				if(status == 3){
					if(isUA(msg) == TRUE){
						STOP = TRUE;
						printf("Connection aknowledged\n");
					}
				}
					
			}
		}while(again == TRUE && STOP == FALSE);
		
	}


	int gif = open("pinguim.gif", O_RDONLY);
	createStartFrame(gif);


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
