/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
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


volatile int STOP=FALSE;
unsigned int timeouts = 0;
unsigned int again = FALSE;


void alarm_handler(int signo)
{
    timeouts++;
    printf("timeouts: %d\n", timeouts);
    again = FALSE;
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
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

	char msg[255];
	sprintf(msg, "%x%x%x%x%x\n", 
			FLAG,ADDRESS_SEND, CONTROL, ADDRESS_SEND^CONTROL, FLAG	);
	res = write(fd, msg, 255);
	printf("%s\n", msg);
	alarm(3);
	char ans[255];
	res = 0;
	int byteRead = 0;
	while((res <= 0) && (timeouts < 3)){
		
		again = 1;
		alarm(3);
		do{
			res = read(fd, ans, 1);
			//printf("%s ", ans);
			
			if(res >0){	
				printf("ansX: %x \n",ans);
				printf("ansD: %d \n",ans);
			}	
			if(strcmp(ans, "%x", 0x7E)>= 0)
				STOP == TRUE;
		}while(again == TRUE && STOP == FALSE);
		
		
	}
	printf("res = %d\n", res);
	
	printf("leu: %x\n", ans);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
