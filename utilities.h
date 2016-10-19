#define BAUDRATE 		B38400
#define MODEMDEVICE 	"/dev/ttyS1"
#define _POSIX_SOURCE 	1 /* POSIX compliant source */
#define FALSE 			0
#define TRUE 			1

#define COUNTER_MODULE	0x40

#define FLAG 			0x7E
#define ADDRESS_SEND 	0x03
#define ADDRESS_RECV 	0x01
//#define BCC 			0x02

#define	CTRL_SET		0x03
#define	CTRL_DISC		0x0B
#define	CTRL_UA			0x07

#define I_CTRL_DATA		0x01
#define I_CTRL_START	0x02
#define I_CTRL_END		0x03

//#define	CTRL_RR(ns)	(ns == 0 ? 0x0)			



typedef enum{
	TRANSMITTER, RECEIVER
}ConnectionMode;


typedef enum {
	SET, DISC, UA, RR, REJ, NONE
}ControlType;

struct applicationLayer {
	int fd;			/*Descritor correspondente à porta série*/
	ConnectionMode transmission;	/*TRANSMITTER | RECEIVER*/
};

struct linkLayer {
	char port[20]; 	/*Dispositivo /dev/ttySx, x = 0, 1*/
	int baudRate;	/*Velocidade de transmissão*/
	unsigned int sequenceNumber;  	 /*Número de sequência da trama: 0, 1*/
	unsigned int timeout;			 /*Valor do temporizador: 1 s*/
	unsigned int numTransmissions;	 /*Número de tentativas em caso de falha*/
	char frame[20];			/*Trama*/
};
