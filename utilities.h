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

#define BAUDRATE 		B38400
#define MODEMDEVICE 	"/dev/ttyS1"
#define _POSIX_SOURCE 		1 /* POSIX compliant source */
#define FALSE 				0
#define TRUE 				1

#define FLAG 				0x7E
#define ADDRESS_SEND 		0x03
#define ADDRESS_RECV 		0x01

#define	SET				0x03
#define	DISC			0x0B
#define	UA				0x07
#define RR				0x05
#define	REJ				0x01
#define NS 				0x40
#define NR 				0x80

#define I_CTRL_DATA			0x01
#define I_CTRL_START		0x02
#define I_CTRL_END			0x03

#define SU_FRAME_SIZE		5
#define CONTROL_PCK_SIZE	10

#define DATA				1
#define START				2
#define END					3




typedef enum{
	TRANSMITTER, RECEIVER
}ConnectionMode;

struct applicationLayer {
	int fd;			/*Descritor correspondente à porta série*/
	ConnectionMode transmission;	/*TRANSMITTER | RECEIVER*/
};

typedef enum {
	BEGIN, TRANSFERING, DISCONNECT, TERMINATE
}State;

struct linkLayer {
	char port[20]; 	/*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Velocidade de transmissão*/
	unsigned int sequenceNumber;  	 /*Número de sequência da trama: 0, 1*/
	unsigned int timeout;			 /*Valor do temporizador: 1 s*/
	unsigned int numTransmissions;	 /*Número de tentativas em caso de falha*/
	char frame[20];			/*Trama*/
	unsigned int maxFrames;			/*numero de frames enviados em cada packet*/
};
