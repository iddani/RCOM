/*Non-Canonical Input Processing*/

#include "utilities.h"

volatile int STOP=FALSE;
volatile int waitFlag = TRUE;
volatile int again = TRUE;
unsigned int packetSize = 100;
unsigned int frameSize = packetSize + 8; //por isto depois de ler do user ou assim
volatile int timeouts = 0;
unsigned int ns = 0x00;
unsigned int numTrama = 0;
struct termios oldtio,newtio;
struct applicationLayer appLayer;
struct linkLayer lLayer;


void signalHandlerIO(int status){

	waitFlag = FALSE;
}

void alarmHandler(int signo) {
    timeouts++;
    printf("timeouts: %d\n", timeouts);
    again = FALSE;
}

void resetTimer(){
	timeouts = 0;
	alarm(0);
}

char *getBCC(char *packet, int length){
		int i;
		char bcc = malloc(sizeof(char) * length);
		for (i = 0; i < length; i++) {
				bcc[i] = (packet[i]^0xFF);
		}
		return bcc;
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
    return -1;
}

int byteStuffing(int fd, char *data){

	int it = 0;
	//char * data = malloc(sizeof(char) * lLayer.maxFrames * 2);
    int res;

	do {
		char byte;
		res = read(fd, &byte, 1);

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

	} while(it < 200);	//numBytes
    return 0;
}

char * byteDestuffing(char * data){

	char * destuffed = malloc(sizeof(char) * frameSize);
	int i = 0;
	for (; i < frameSize; ++i){

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

char analyse(char msg[]){

	char address = msg[1], control = msg[2], bcc = msg[3];
	if(bcc != (address^control)){
		return -1;
	} else {
		return control;
	}
}

int createSUFrame(char type, char *frame){

	frame[0] = FLAG;
	frame[4] = FLAG;

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
			frame[2] = SET;
			break;
		case DISC:
			frame[2] = DISC;
			break;
		case UA:
			frame[2] = UA;
			break;
		case RR:

			break;
		case REJ:

			break;
        default:
            break;
	}
    frame[3] = (frame[1]^frame[2]);
	return 0;
}

int sendSUFrame(char type){

	int res = 0;
	char frame[SU_FRAME_SIZE];
	createSUFrame(type, frame);
	res = write(appLayer.fd, frame, SU_FRAME_SIZE);
	printf("Wrote SU frame with %d bytes to fd\n", res);
    return 0;
}

int readMessage(char *answer){
    char buf;
	int ansLength = 0, status = 0;
    int res;
    again = TRUE;

    while(again == TRUE){

    	write(1, ".", 1); usleep(100000);
		if(waitFlag == FALSE){
	        if((res = read(appLayer.fd, &buf, 1)) > 0){
	            status = readByte(buf, status);
	            answer[ansLength] = buf;
	            ansLength++;

	            if(status == 3){
	                if(answer[3]==(answer[1]^answer[2])){
	                	again = FALSE;
	                    return 0;
	                }
	            }
	        } else waitFlag = TRUE;
	   	}
    }
    return -1;
}

char makeBCC2(char * packet){

	char lixo;
	return lixo;

}

char * createIframe(char *packet, int numPacketBytes){

	char * frame = malloc(sizeof(char) * frameSize);

	frame[0] = FLAG;
	frame[1] = ADDRESS_SEND;
	frame[2] = numTrama;
	frame[3] = frame[1]^frame[2];

	memcpy(&frame[4], packet, numPacketBytes);

	frame[5] = makeBCC2(packet);
	frame[6] = FLAG;

}

char *createDataPacket(int fd){ // bytestufs maxFrames from file
    char *data = malloc(sizeof(char) * lLayer.maxFrames * 2);
    byteStuffing(fd, data);
    return data;
}

char *codeFileSize(int fileSize, int *num){
	int nBytes = 4, remainder, iterator = 0;
	char reverse[nBytes];


	do {
		remainder = fileSize % 0x100;
		fileSize /= 0x100;
		printf("remainder %x\n",(unsigned char) remainder);
		if(remainder > 0){
			reverse[(*num)] = remainder;
			printf("%x\n", (unsigned char) reverse[(*num)]);
			(*num)++;
		}
		iterator++;
	} while(remainder > 0);

	char *size = malloc(sizeof(char) * (*num));

	int i;
	for (i = 0; i < (*num); ++i) {
		size[i] = reverse[(*num)-1-i];
	}

	return size;
}

char *createControlPacket(int type, int fd, int *pacLength){

	struct stat s;
	if (fstat(fd, &s) == -1) {
  		printf("fstat(%d) returned error=%d.", fd, errno);
	}
	int size = s.st_size;

	char *packet = malloc(sizeof(char) * (*pacLength));
	packet[0] = type;		//control
	packet[1] = 0x00;		//type -> size


	int nBytes = 0;
	char *codedSize = codeFileSize(size, &nBytes);
	if((*pacLength) < ((*pacLength) + nBytes-1)){
		(*pacLength) += nBytes - 1;
		packet = realloc(packet, (*pacLength));
<<<<<<< HEAD
=======

>>>>>>> bbda746ac5c0b2f5c9e2541a763328d92121ee13
	}

	packet[2] = nBytes;		//number of bytes
	memcpy(&(packet[3]), codedSize, nBytes);
	packet[3 + nBytes] = 0x01;		//type -> name

	int nameLength = strlen("pinguim.gif");
	if((*pacLength) < ((*pacLength) + nameLength - 1)){
		(*pacLength) += nameLength - 1;
		packet = realloc(packet, (*pacLength));

	}
	packet[4 + nBytes] = nameLength;		//number of bytes
	memcpy(&(packet[5 + nBytes]), "pinguim.gif", nameLength);

	return packet;
}

char *createIFrameControl(int type, int fd){

	//fileInfo
	int pacSize = 7;
	char *controlPacket = createControlPacket(type, fd, &pacSize);

	char *packet = malloc(sizeof(char) * 30);
	packet[0] = FLAG;

	packet[1] = ADDRESS_SEND;
	ns = (ns^COUNTER_MODULE);
	packet[2] = ns;
	packet[3] = (packet[1]^packet[2]);	//BCC

	memcpy(&packet[4], controlPacket, pacSize);
	char *bcc = getBCC(controlPacket, pacSize);
	memcpy(&packet[4 + pacSize], bcc, pacSize);
	packet[4 + 2*pacSize] = FLAG;

	free(bcc);
	free(controlPacket);
	return packet;
}

int llwrite(int fd, char* msg, int length){

	STOP = FALSE;

	while(STOP == FALSE){
    int res = byteStuffing(fd, data); //maxbytes de cada vez

		if(res > 0){

				createDataPacket

		}

	}


  return res;
}

int llread(){

	while (STOP == FALSE) {
		int size;
		char *msg = readIFrame(&size);

		if(TRUE){	//sem erros detectados no cabeçalho e no campo de dados
			char *destuffed = byteDestuffing(msg, size);
			sendSUFrame(RR);
		}
	}
	readMessage()
    return 0;
}

int llopen(int gate, ConnectionMode connection){

	if(connection == TRANSMITTER){

		STOP = FALSE;
		int res = 0;
		char answer[20];
	    while (STOP == FALSE && timeouts < 3) {
	        sendSUFrame(SET);
	        alarm(3);
	        res = readMessage(answer);

			if(analyse(answer) == UA){
				printf("Recognized UA\n");
				STOP = TRUE;
			}
	    }
	}
	else {
		char msg[20];
		int res;

		readMessage(msg);
		if(analyse(msg) == SET){
			sendSUFrame(UA);
			STOP = TRUE;
			printf("Setup aknowlegded. Sending UA\n");
		}
	}
	resetTimer();
    return gate;
}

/*  Envia DISC e espera por UA ou por DISC dependendo se emite ou recebe*/
int llclose(int fd){
	char answer[5];
	STOP = FALSE;
    if(appLayer.transmission == TRANSMITTER){

	    while(STOP == FALSE && timeouts < 3){

		    sendSUFrame(DISC);
	        alarm(3);
	        readMessage(answer);
	        if(analyse(answer) == DISC){
	        	printf("Received DISC sending UA\n");
	            sendSUFrame(UA);
	            STOP = TRUE;
	        }
   		}
    } else if(appLayer.transmission == RECEIVER){

    	readMessage(answer);
        if(analyse(answer) == DISC){
            while(STOP == FALSE){
			    sendSUFrame(DISC);
		        alarm(3);
		        readMessage(answer);

		        if(analyse(answer) == UA){
		            STOP = TRUE;
		        }
   			}
        }
    }
    resetTimer();
    return 0;
}

/*  Envio de informacao do lado do emissor*/
int transfer(){

    char *msg;
    int fd = open("pinguim.gif", O_RDONLY);
    if(fd < 0){
    	printf("Error opening file. Error: %d\n", fd);
    }

    //msg = createSUFrame(START);
    createDataPacket(fd);

    close(fd);
    return 0;
}

int receive(){

    return 0;
}

int main(int argc, char** argv) {

    //int fd;
    int sum = 0, speed = 0;


    if ( (argc < 3) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort ConnectionMode Ba\n\tex: nserial /dev/ttyS1 TRANSMITTER||RECEIVER\n");
      exit(1);
    }

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

    appLayer.fd = open(argv[1], O_RDWR | O_NOCTTY );

    if (appLayer.fd <0) {perror(argv[1]); exit(-1); }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  	*/
    if ( tcgetattr(appLayer.fd,&oldtio) == -1) { /* save current port settings */
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

    struct sigaction actionAlarm;
    actionAlarm.sa_handler = alarmHandler;
    sigemptyset(&actionAlarm.sa_mask);
    actionAlarm.sa_flags = 0;

    if (sigaction(SIGALRM,&actionAlarm, NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGALRM handler\n");
        exit(1);
    }


	struct sigaction actionIO;
    actionIO.sa_handler = signalHandlerIO;
    actionIO.sa_flags = 0;
    if (sigaction(SIGIO,&actionIO, NULL) < 0)
    {
        fprintf(stderr,"Unable to install SIGIO handler\n");
        exit(3);
	}

    fcntl(appLayer.fd, F_SETOWN, getpid());
    fcntl(appLayer.fd, F_SETFL, FASYNC);




    tcflush(appLayer.fd, TCIOFLUSH);

    if ( tcsetattr(appLayer.fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);

    }

    printf("New termios structure set\n");
    llopen(gate, appLayer.transmission);
    printf("Opened correctly. Starting close\n");
	int gif = open("pinguim.gif", O_RDONLY);
    char * packet = createIFrameControl(START, gif);
    free(packet);
    llclose(appLayer.fd);



	/*
	int gif = open("pinguim.gif", O_RDONLY);

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

    if ( tcsetattr(appLayer.fd,TCSANOW,&oldtio) == -1) {

      perror("tcsetattr");
      exit(-1);
    }

    close(appLayer.fd);
    return 0;
}
