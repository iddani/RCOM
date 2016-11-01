/*Non-Canonical Input Processing*/
#include <math.h>
#include "utilities.h"

volatile int STOP=FALSE;
volatile int waitFlag = TRUE;
volatile int again = TRUE;
unsigned int totalRead = 0;
volatile int timeouts = 0;
unsigned int ns = 0x00, nr = 0x00;
unsigned int packetSeq = 0;
unsigned int numTrama = 0;
unsigned int eof = FALSE;
char *fileName;
struct termios oldtio,newtio;
struct applicationLayer appLayer;
struct linkLayer lLayer;
int total =0;
State state;


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

char getBCC2(char *packet, int length){		//descobre o bcc2 pra retornar
	int i;
	char bcc;
	for (i = 0; i < (length); i++) {
			bcc = bcc ^ packet[i];
	}
	return bcc;
}

int checkBCC2(char *packet, int length){	//inclui o bcc2 no packet
	int i;
	char bcc = 0x00;
	for (i = 0; i < (length - 1); i++) {
			bcc = bcc ^ packet[i];
	}
	if(bcc == packet[length -1]){
		return TRUE;
	} else{
		return FALSE;
	}
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

void byteStuffing(char *data, int *it, char byte){
	if(byte == FLAG){
		data[*it] = 0x7D;
		data[++(*it)] = 0x5E;
		printf("STUFFEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDdd %d\n", (*it));
	} else if (byte == 0x7D){
		data[*it] = 0x7D;
		data[++(*it)] = 0x5D;
		printf("STUFFEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDdd %d\n", (*it));
	} else{
		data[*it] = byte;
	}
	(*it)++;
}

char * byteDestuffing(char *data, int msgLength, int * destuffedLength){
	char * destuffed = malloc(sizeof(char) * msgLength);
	int i = 4, j = 0;
	while(data[i] != FLAG){
		if(data[i] == 0x7D && data[i+1] == 0x5E){
			destuffed[j++] = 0x7E;
			i+=2;
		} else if(data[i] == 0x7D && data[i+1] == 0x5D){
			destuffed[j++] = 0x7D;
			i+=2;
		} else {
			destuffed[j++] = data[i++];
		}
	}
	(*destuffedLength) = j;
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
	switch(appLayer.transmission){
		case TRANSMITTER:
			frame[1] = ADDRESS_SEND;
			break;
		case RECEIVER:
			frame[1] = ADDRESS_SEND;
			break;
	}

	frame[2] = type;
    frame[3] = (frame[1]^frame[2]);
    frame[4] = FLAG;
	return 0;
}

int sendSUFrame(char type){

	int res = 0;
	char frame[SU_FRAME_SIZE];
	createSUFrame(type, frame);
	res = write(appLayer.fd, frame, SU_FRAME_SIZE);
    return 0;
}

char *codeFileSize(int fileSize, int *num){
	int nBytes = 4, rem, iterator = 0;
	char *size = malloc(sizeof(char) * nBytes);

	do {
		rem = fileSize % 0x100;
		fileSize /= 0x100;
		if(rem > 0){
			size[(*num)++] = (unsigned char)rem;
		}
		iterator++;
	} while(rem > 0);
	return size;
}

char *readMessage(int *size){
    char buf;
	int msgLength = 0, status = 0;
    int res;

    again = TRUE;
	//waitFlag = TRUE;
    char *msg = malloc(sizeof(char) *(*size));

    while(again == TRUE){

		//if(waitFlag == TRUE)
    	//	write(1, ".", 1); usleep(100000);
		if(waitFlag == FALSE){
	        if((res = read(appLayer.fd, &buf, 1)) > 0){
	            status = readByte(buf, status);
				if(msgLength >= (*size)){
					(*size) += 50;
					msg = realloc(msg, (*size));
				}
	            msg[msgLength] = buf;
	            msgLength++;

	            if(status == 3){
	                if(msg[3] == (msg[1]^msg[2]) ){
	                	again = FALSE;
						(*size) = msgLength;
	                    return msg;
	                }else {
						printf("error in read msg\n");
						msgLength = 0;
						status = 0;
					}
	            }
	        } else if (res == 0) {
				printf("Res deu 0!\n" );
				waitFlag = TRUE;
			}
	   	}
    }
    return NULL;
}

char *makePayload(int fd, char *bcc, int *it){ //BCC2 + Stuffing
	char *data = malloc(sizeof(char) * lLayer.maxFrames * 2);
 	int res, i = 0;

	do {
		char byte;
		res = read(fd, &byte, 1);
		if(res == 0) {
			eof = TRUE;
			lLayer.maxFrames = i;
			printf("packet: %d\n", lLayer.maxFrames);
			break;
		}
		else if(res < 0){
			printf("Error reading from gif no %d\n", errno);
			return NULL;
		}
		(*bcc) = (*bcc)^byte;
		byteStuffing(data, it, byte);
		i++;

	} while(i < lLayer.maxFrames);

	totalRead += i;
    return data;
}

char *createControlPacket(int fd, int type, int *pacLength){

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
	}

	packet[2] = nBytes;		//number of bytes
	memcpy(&packet[3], codedSize, nBytes);
	packet[3 + nBytes] = 0x01;		//type -> name

	int nameLength = strlen("pinguim.gif");
	if((*pacLength) < ((*pacLength) + nameLength - 1)){
		(*pacLength) += nameLength - 1;
		packet = realloc(packet, (*pacLength));
	}
	packet[4 + nBytes] = nameLength;		//number of bytes
	memcpy(&(packet[5 + nBytes]), "pinguim.gif", nameLength);

	char bcc = getBCC2(packet, (*pacLength));

	if((*pacLength) < ((*pacLength) + 1)){
		(*pacLength) += 1;
		packet = realloc(packet, (*pacLength));
	}

	packet[(*pacLength)-1] = bcc;
	return packet;
}

char *createDataPacket(int fd, int *length){ //com data stuffed, flags nao stuffed e bcc sobre isso

	char * packet = malloc(sizeof(char) * lLayer.maxFrames * 2);
	int it =0;
	int num = 0;

	char bcc = DATA ^ packetSeq;

	byteStuffing(packet, &it, DATA);
	byteStuffing(packet, &it, packetSeq++);
	if(packetSeq > 255){
		packetSeq = 0;
	}

	char *stuffed = makePayload(fd, &bcc, length);
	char * nBytes = codeFileSize(lLayer.maxFrames, &num);
	byteStuffing(packet, &it, nBytes[0]);
	if(num < 2) nBytes[1] = 0;
	byteStuffing(packet, &it, nBytes[1]);

	bcc = bcc  ^ nBytes[0] ^ nBytes[1];
	free(nBytes);

	if((lLayer.maxFrames * 2) < (4+(*length))){
		packet = realloc(packet, 20+(*length));
	}

	memcpy(&packet[it], stuffed, *length);
	it += (*length);
	byteStuffing(packet,&it, bcc);
	packet[it] = FLAG;
	free(stuffed);
	(*length) = it;
	printf("%x\n", (unsigned char)packet[17]);
	return packet;

}

int sendIFrame(int fd, char *frame, int size){
	char *msg;
	while (STOP == FALSE && timeouts < 3) {
		int res = write(appLayer.fd, frame, size);
		printf("Wrote I frame with %d bytes to fd\n", res);
		alarm(lLayer.timeout);

		msg = readMessage(&size);

		if(msg != NULL){
			if(analyse(msg) == (RR)){
				//nr = nr^NR;
				resetTimer();
				free(msg);
				return 0;
			}
		}
	}
	return -1;
}

char *createIFrame(int fd, int type, int *frameSize){ //adiciona flags, chama makeplayload

	char *frame = malloc(sizeof(char) * 20);
	char *packet, *stuffed;
	int length = 7;

	frame[0] = FLAG;
	frame[1] = ADDRESS_SEND;
	frame[2] = NS;
	//ns = ns^NS;
	frame[3] = frame[1]^frame[2];

	switch (type) {
		case START:
		case END:
			packet = createControlPacket(fd, type, &length);	//contem ja bcc
			stuffed = malloc(sizeof(char) * length * 2);

			int i = 0, j = 0;
			while(j < length){
				byteStuffing(stuffed, &i, packet[j++]);
			}
			free(packet);

			length = i;
			if(20 < (5+length)){
				frame = realloc(frame, 5+length);
			}
			memcpy(&frame[4], stuffed, length);
			break;

		case DATA:
			length = 0;
			stuffed = createDataPacket(fd, &length);
			if(20 < (5+length)){
				frame = realloc(frame, 6+length);
			}
			memcpy(&frame[4], stuffed, length);
			break;
	}
	frame[4+length] = FLAG;
	free(stuffed);
	(*frameSize) = (5+length);

	return frame;
}

int readDataPacket(char *msg, int *pacSequence){
    int fd = open(fileName, O_APPEND | O_WRONLY);

	int sequenceNumber = msg[1];
	printf("Sequence number %d\n", sequenceNumber);
	if(sequenceNumber != ((*pacSequence + 1) % 256)){	//last sequence number
		if(sequenceNumber == (*pacSequence % 256)){
			printf("asdasdasdasdadasdada\n");
			return 0;
		} else {

		}
	}
	int nBytes = (unsigned char)msg[3] * 0x100 + (unsigned char)msg[2];
    int res;
    res = write(fd, &msg[4], nBytes);
	total += res;
    printf("Wrote %d bytes to file\n", res);
	close(fd);
	return 0;
}

int readControlPacket(char *msg, int *fileSize){
	int size = 0, i;
	char sizeBytes, nameBytes;
	sizeBytes = msg[2];

	for (i = 0; i < sizeBytes; i++) {
        int expon= 1, j;
	    for ( j = 0; j < i; j++) {
            expon *= 0x100;
        }
		size += ((unsigned char)msg[3+i] * expon);
	}
    printf("File size: %d\n", size);

	nameBytes = msg[4+sizeBytes];
    if(msg[0] == START){
		fileName = malloc(nameBytes * sizeof(char));
		memcpy(fileName, &msg[5+sizeBytes], nameBytes);
        close(open(fileName, O_CREAT | O_TRUNC | O_WRONLY, 00644));
	} else {
		int fd = open(fileName, O_RDONLY);
		struct stat s;
		if (fstat(fd, &s) == -1) {
	  		printf("fstat(%d) returned error=%d.", fd, errno);
		}
		if((int)s.st_size != size){
			printf("Warning file size different from size in packet\n");
		}
		close(fd);
	}
	(*fileSize) = size;
	return 0;
}

int llread(char *msg, int msgLength){
	printf("Read %d bytes from serial port\n", msgLength);
	int destuffedLength, pacSequence = -1;
	char *dataDestuffed = byteDestuffing(msg, msgLength, &destuffedLength);

	if(checkBCC2(dataDestuffed, destuffedLength) == FALSE){
		printf("Error confirming BCC2.\n");
		return -1;
	}

	int type = dataDestuffed[0];
	printf("Type: %x\n", type);
	int size;
	switch(type){
		case START:
			readControlPacket(dataDestuffed, &size);
			break;
		case DATA:
			readDataPacket(dataDestuffed, &pacSequence);
			break;
		case END:
			readControlPacket(dataDestuffed, &size);
			state = DISCONNECT;
			break;
		default:
			return -1;
	}

	free(dataDestuffed);
	return 0;
}

int llwrite(int fd){

	STOP = FALSE;
	char *frame;
	int type = START, size, res;

	while(STOP == FALSE && timeouts < lLayer.numTransmissions){
		STOP = FALSE;
		frame = createIFrame(fd, type, &size);
		res = sendIFrame(fd, frame, size);
		switch (type) {
			case START:
				if(res == 0){
					printf("Acknowledged START packet\n");
					type = DATA;
				}
				break;
			case DATA:
				if(res == 0){
					if(eof == TRUE){
						type = END;
						printf("Finished file transfer\n");
					}
				}
				break;
			case END:
				if(res == 0){
					printf("Acknowledged END packet\n");
					STOP = TRUE;
				}
				break;
		}
		free(frame);

	}
	return 0;
}

int llopen(){

	int size = CONTROL_PCK_SIZE;
	if(appLayer.transmission == TRANSMITTER){

		STOP = FALSE;
	    while (STOP == FALSE && timeouts < lLayer.numTransmissions) {
	        sendSUFrame(SET);
	        alarm(lLayer.timeout);

	        char *msg = readMessage(&size);
			if(msg != NULL){
				if(analyse(msg) == UA){
					printf("Connection established.\n");
					STOP = TRUE;
					resetTimer();
					free(msg);
					return 0;
				}
			}
	    }
	} else {
		sendSUFrame(UA);
		printf("Setup aknowlegded. Preparing for transfer\n");
		state = TRANSFERING;
		return 0;
	}
    return -1;
}

/*  Envia DISC e espera por UA ou por DISC dependendo se emite ou recebe*/
int llclose(){
	char *msg;
	STOP = FALSE;
	int size;
    if(appLayer.transmission == TRANSMITTER){

	    while(STOP == FALSE && timeouts < lLayer.numTransmissions){

		    sendSUFrame(DISC);
	        alarm(lLayer.timeout);
	        msg = readMessage(&size);
	        if(analyse(msg) == DISC){
	        	printf("Ended transmission\n");
	            sendSUFrame(UA);
	            STOP = TRUE;
	        }
			free(msg);
   		}
    } else if(appLayer.transmission == RECEIVER){

        while(STOP == FALSE && timeouts < lLayer.numTransmissions){
		    sendSUFrame(DISC);
	        alarm(lLayer.timeout);
	        msg = readMessage(&size);

	        if(analyse(msg) == UA){
	            STOP = TRUE;
				printf("Confirmed transmission termination\n" );
	        }
			free(msg);
		}
    }
    resetTimer();
    return 0;
}

/*  Envio de informacao do lado do emissor*/
int transfer(){

    int fd = open("pinguim.gif", O_RDONLY);
    if(fd < 0){
    	printf("Error opening file. Error: %d\n", fd);
		exit(1);
    }
	while(state != TERMINATE){
	    switch(state){
			case BEGIN:
				if(llopen()==0){
					state = TRANSFERING;
					printf("Setting up nice\n");
				} else {
					state = TERMINATE;
					printf("Error setting up. Shutting down\n");
				}
				break;
			case TRANSFERING:
				llwrite(fd);
				printf("Transfered yeah\n");
				state = DISCONNECT;
				break;
			case DISCONNECT:
				llclose();
				printf("disconnected ouiiiiii\n");
				state = TERMINATE;
				break;

			case TERMINATE:
				break;
		}
	}
    close(fd);
    return 0;
}

int receiver(){
	int size = 70, res;
	char t;
	while(state != TERMINATE){
		char *msg = readMessage(&size);
		switch(state){
			case BEGIN:
				if(analyse(msg) == SET){
					res = llopen();
				} else{
					sendSUFrame(REJ);
					//nr = nr^NR;
				}
				break;
			case TRANSFERING:

				if((t=analyse(msg)) == NS){	//control for I frames
					res = llread(msg, size);

					if(res == 0){
						sendSUFrame(RR);
						//nr = nr^NR;
					}
				} else printf("falhou analyse. ctrl: %x\n", t);
				break;
			case DISCONNECT:
				if(analyse(msg) == DISC){
					res = llclose();
					state = TERMINATE;
					printf("OH MY GOD\n");
				} else{
					sendSUFrame(REJ);
					nr = nr^NR;
				}
				break;

			case TERMINATE:
				break;
		}
		free(msg);
	}
	printf("Wrote total of %d to file\n", total);
    return 0;
}

void defaultSettings(){
	lLayer.numTransmissions = 3;
	lLayer.timeout = 3;
	lLayer.baudRate = 0xB38400;
	lLayer.maxFrames = 350;
}

int manualSettings(char *baudRate, char *btf, char *retransmissions, char *timeout){
	char *end;
	lLayer.baudRate = strtoul(baudRate, &end, 16);
	if(*end != '\0'){
		printf("invalid value for BaudRate.\n");
		exit(1);
	}

	lLayer.maxFrames = strtoul(btf, &end, 10);
	if(*end != '\0' || lLayer.maxFrames < 1){
		printf("invalid value for maxFrames.\n");
		exit(1);
	}
	lLayer.numTransmissions = strtoul(retransmissions, &end, 10);
	if(*end != '\0' || lLayer.numTransmissions < 1){
		printf("invalid value for numTransmissions.\n");
		exit(1);
	}
	lLayer.timeout = strtol(timeout, &end, 10);
	if(lLayer.timeout < 1 || lLayer.timeout > 5000 || *end != '\0'){
		printf("invalid value for timeout.\n");
		exit(1);
	}


	printf("baudRate: %x\n", lLayer.baudRate);
	printf("maxFrames: %d\n", lLayer.maxFrames);
	printf("transmission: %d\n", lLayer.numTransmissions);
	printf("timeout: %d\n", lLayer.timeout);

}

int main(int argc, char** argv) {

    if ( (argc < 3) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Default Usage:\tnserial SerialPort ConnectionMode\n\tex: nserial /dev/ttyS1 TRANSMITTER||RECEIVER\n");
	  printf("Manual Usage:\tnserial SerialPort Filename ConnectionMode BaudRate BytesPerFrame NumTransmissions Timeout\n\tex: nserial /dev/ttyS1 pinguim.gif  TRANSMITTER||RECEIVER <int> <int> <int> <int>\n");
      exit(1);
  } else{
	  sprintf(lLayer.port, "%s", argv[1]);
  }

    if(strcmp("TRANSMITTER", argv[2])==0){
    	appLayer.transmission = TRANSMITTER;
	} else if(strcmp("RECEIVER", argv[2])==0){
		appLayer.transmission = RECEIVER;
	} else{
		printf("ConnectionMode not recognized\n\tuse: TRANSMITTER||RECEIVER\n");
      		exit(1);
	}

	if(argc == 3){
		defaultSettings();
	} else if(argc == 7){
		manualSettings(argv[3], argv[4], argv[5], argv[6]);
		return 0;
	}

    appLayer.fd = open(lLayer.port, O_RDWR | O_NOCTTY );

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
    newtio.c_cflag = lLayer.baudRate | CS8 | CLOCAL | CREAD;
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

	if(appLayer.transmission == TRANSMITTER){
		transfer();
	} else{
		receiver();
	}
	printf("Total bytes read from file: %d\n", totalRead);
    if ( tcsetattr(appLayer.fd,TCSANOW,&oldtio) == -1) {

      perror("tcsetattr");
      exit(-1);
    }

    close(appLayer.fd);
    return 0;
}
