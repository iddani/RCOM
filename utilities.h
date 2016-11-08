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

#define SU_FRAME_SIZE		5
#define CONTROL_PCK_SIZE	10

#define DATA				1
#define START				2
#define END					3

typedef enum{
	TRANSMITTER, RECEIVER
}ConnectionMode;

struct applicationLayer {
	int fd;			/*Descritor correspondente ao ficheiro*/
	ConnectionMode transmission;	/*TRANSMITTER || RECEIVER*/
	unsigned int packetSeq; /*numero de sequencia do pacote de dados*/
};

typedef enum {
	BEGIN, TRANSFERING, DISCONNECT, TERMINATE
}State;	/*Estados de um programa*/

struct linkLayer {
	char port[20]; 	/*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Velocidade de transmissão*/
	unsigned int sequenceNumber;  	 /*Número de sequência da trama: 0, 1*/
	unsigned int timeout;			 /*Valor do temporizador: 1 s*/
	unsigned int numTransmissions;	 /*Número de tentativas em caso de falha*/
	unsigned int maxFrames;			/*numero de frames enviados em cada packet*/

	volatile int numTimeouts;		/*numero de vezes que o temporizador ativou na espera actual*/
	unsigned int ns, nr;			/*contadores do transmissor e emissor nas tramas I e U*/
};

int checkFile(char *fileName);
int checkBaudRate(int input);
