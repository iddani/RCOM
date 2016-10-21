/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define ADDRESS	0x03
#define CONTROL 0x07
#define BCC 0x06



volatile int STOP=FALSE;
volatile int waitFlag = TRUE;

void signalHandlerIO( int status){
	waitFlag = FALSE;
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

	return 0;

}

int isSetup(char *msg){
	if(msg[3] != (msg[2]^msg[1])){
		return -1;	//back to start
	} else{
		if(msg[2] == 0x03){ //para setup
			return TRUE;
		} else{
			return 0;	//proceed for different control
		}
	}
}

char analyse(char msg[]){

	char address = msg[1], control = msg[2], bcc = msg[3];
	printf("%x\n", address);
	printf("%x\n", control);
	printf("%x\n", bcc);
	if(bcc != (address^control)){
		return -1;
	} else {
		return control;
	}
}

int sendUA(int fd){
	char responseUA[5];
	responseUA[0] = FLAG;
	responseUA[1] = ADDRESS;
	responseUA[2] = CONTROL;
	responseUA[3] = (ADDRESS^CONTROL);
	responseUA[4] = FLAG;

	write(fd,responseUA,5);
	return 0;
}

/*  Envia DISC e espera por UA ou por DISC dependendo se emite ou recebe*/
int llclose(int fd){
    char answer[5];
	char msg[5];// = createSUFrame(DISC);
	
    while(STOP == FALSE){
        	STOP = FALSE;
        	readMessage(answer);
    	
        	if(analyse(answer) == CTRL_DISC){
            	createSUFrame(CTRL_DISC, msg);
            	while(STOP == FALSE){
	    			sendMessage(msg);
        			//alarm(3);
        			readMessage(answer);
    	
        			if(analyse(answer) == CTRL_UA){
            			close(fd);
						STOP = TRUE;
       				}
				}
       		}
		}
    //free(msg);
   
   
    
    return 0;
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;

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

		
	struct sigaction action;
    action.sa_handler = signalHandlerIO;
    if (sigaction(SIGIO,&action, NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGIO handler\n");
        exit(3);
	}

    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, FASYNC);





  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */

/* TP1*/

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


	char msg[20];
	int msgLength = 0, status = 0;
	char buf;
	int a;

	while (STOP==FALSE) {       // loop for input 
		write(1, ".", 1); usleep(100000);

		if(waitFlag == FALSE){
			if((res = read(fd,&buf,1)) > 0){   // returns after 0 chars have been input 

				status = readByte(buf, status);
				msg[msgLength] = buf;
				msgLength++;

				if(status == 3){
					if(isSetup(msg) == TRUE){
						sendUA(fd);
						STOP = TRUE;

						printf("Setup aknowlegded. Sending UA\n");
					} else {
						msgLength = 0;
					}
				}

			}
			else if (res < 0){
				printf("hey %d\n", errno);
			}
		}
    }

    llclose();

  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}


