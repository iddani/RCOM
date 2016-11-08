#include "utilities.h"

volatile int STOP = FALSE;
volatile int waitFlag = TRUE;
volatile int again = TRUE;
unsigned int eof = FALSE;
unsigned int numRej = 0, numTimeout = 0, numI = 0, total = 0;
char *fileName;
unsigned int fileSize;
struct applicationLayer appLayer;
struct linkLayer lLayer;
State state;

void signalHandlerIO(int status){

	waitFlag = FALSE;
}

void alarmHandler(int signo) {
	numTimeout++;
    lLayer.numTimeouts++;
    printf("lLayer.numTimeouts: %d\n", lLayer.numTimeouts);
    again = FALSE;
}

void resetTimer(){
	lLayer.numTimeouts = 0;
	alarm(0);
}

void resetStatistics(){
	numRej = 0; numTimeout = 0; numI = 0;
}

/*
	======================= applicationLayer ==========================
*/

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

int isDuplicate(char *msg){
	int sequenceNumber = msg[1];
	if(sequenceNumber == (appLayer.packetSeq % 256)){
		printf("Found duplicate: %d", sequenceNumber);
		return TRUE;
	}
	return FALSE;
}

char getBCC2(char *packet, int length){		//descobre o bcc2 pra retornar
	int i;
	char bcc = 0x00;
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

char *makePayload(){ //BCC2 + Stuffing
	char *data = malloc(sizeof(char) * lLayer.maxFrames);
 	int res, i = 0;

	do {
		char byte;
		res = read(appLayer.fd, &byte, 1);
		if(res == 0) {
			eof = TRUE;
			lLayer.maxFrames = i;
			break;
		}
		else if(res < 0){
			printf("Error reading from file:  %d\n", errno);
			return NULL;
		}
		data[i++] = byte;


	} while(i < lLayer.maxFrames);

	total += i;
    return data;
}

char *createControlPacket(int type, int *pacLength){

	struct stat s;
	if (fstat(appLayer.fd, &s) == -1) {
  		printf("fstat(%d) returned error=%d.", appLayer.fd, errno);
	}
	fileSize = s.st_size;

	char *packet = malloc(sizeof(char) * (*pacLength));
	packet[0] = type;		//control
	packet[1] = 0x00;		//type -> size

	int nBytes = 0;
	char *codedSize = codeFileSize(fileSize, &nBytes);
	if((*pacLength) < ((*pacLength) + nBytes-1)){
		(*pacLength) += nBytes - 1;
		packet = realloc(packet, (*pacLength));
	}

	packet[2] = nBytes;		//number of bytes
	memcpy(&packet[3], codedSize, nBytes);
	packet[3 + nBytes] = 0x01;		//type -> name

	int nameLength = strlen(fileName);
	if((*pacLength) < ((*pacLength) + nameLength - 1)){
		(*pacLength) += nameLength - 1;
		packet = realloc(packet, (*pacLength));
	}
	packet[4 + nBytes] = nameLength;		//number of bytes
	memcpy(&(packet[5 + nBytes]), fileName, nameLength);

	char bcc = getBCC2(packet, (*pacLength));

	if((*pacLength) < ((*pacLength) + 1)){
		(*pacLength) += 1;
		packet = realloc(packet, (*pacLength));
	}

	packet[(*pacLength)-1] = bcc;
	return packet;
}

char *createDataPacket(){ //com data stuffed, flags nao stuffed e bcc sobre isso

	char * packet = malloc(sizeof(char) * (lLayer.maxFrames + 4));
	int num = 0;

	packet[0] = 1;
	appLayer.packetSeq++;
	if(appLayer.packetSeq > 255){
		appLayer.packetSeq = 0;
	}
	packet[1] = appLayer.packetSeq;

	char *data = makePayload();
	char * nBytes = codeFileSize(lLayer.maxFrames, &num);

	packet[2] = nBytes[0];
	if(num < 2) nBytes[1] = 0;
	packet[3] = nBytes[1];

	free(nBytes);
	memcpy(&packet[4], data, lLayer.maxFrames);
	free(data);
	return packet;
}

int readDataPacket(char *msg){

	if(isDuplicate(msg) == TRUE){
		return 0;
	} else appLayer.packetSeq++;

    appLayer.fd = open(fileName, O_APPEND | O_WRONLY);

	int sequenceNumber = (unsigned char)msg[1];
	printf("Sequence number %d\n", (unsigned int)sequenceNumber);

	int nBytes = (unsigned char)msg[3] * 0x100 + (unsigned char)msg[2];
	int res;
	res = write(appLayer.fd, &msg[4], nBytes);
	total += res;

	printf("Wrote %d bytes to file. (%d%%)\n", res, (total* 100)/fileSize);

	close(appLayer.fd);
	return 0;
}

int readControlPacket(char *msg){
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
		fileName = malloc((1+nameBytes) * sizeof(char));
		memcpy(fileName, &msg[5+sizeBytes], nameBytes);
		fileName[(int)nameBytes] = '\0';
        close(open(fileName, O_CREAT | O_TRUNC | O_WRONLY, 00644));
	} else {
		appLayer.fd = open(fileName, O_RDONLY);
		struct stat s;
		if (fstat(appLayer.fd, &s) == -1) {
	  		printf("fstat(%d) returned error=%d.", appLayer.fd, errno);
		}
		if((int)s.st_size != size){
			printf("Warning file size different from size in packet\n");
		}
		close(appLayer.fd);
	}
	fileSize = size;
	return 0;
}

int analyseIFrameData(char *dataDestuffed, int type){
	switch(type){
		case START:
			readControlPacket(dataDestuffed);
			break;
		case DATA:
			readDataPacket(dataDestuffed);
			break;
		case END:
			readControlPacket(dataDestuffed);
			state = DISCONNECT;
			break;
		default:
			return -1;
	}
	return 0;
}

/*
  	======================= linkLayer ================================
*/

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

char *byteStuffing(char *data, int msgLength, int *stuffedLength){
	char *stuffed = malloc(sizeof(char) * (msgLength)*2);
	int i, it = 0;
	for ( i = 0; i < msgLength; i++) {
		if(data[i] == FLAG){
			stuffed[it] = 0x7D;
			stuffed[++it] = 0x5E;
		} else if (data[i] == 0x7D){
			stuffed[it] = 0x7D;
			stuffed[++it] = 0x5D;
		} else{
			stuffed[it] = data[i];
		}
		it++;
	}
	(*stuffedLength) = it;
	return stuffed;

}

char * byteDestuffing(char *data, int msgLength, int *destuffedLength){
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

int sendSUFrame(int fd, char type){

	char frame[SU_FRAME_SIZE];
	createSUFrame(type, frame);
	write(fd, frame, SU_FRAME_SIZE);
    return 0;
}

char *readMessage(int fd, int *size){
    char buf;
	int msgLength = 0, status = 0;
    int res;

    again = TRUE;
    char *msg = malloc(sizeof(char) *(*size));

    while(again == TRUE){

		if(waitFlag == FALSE){
	        if((res = read(fd, &buf, 1)) > 0){
	            status = readByte(buf, status);
				if(status == 1){
					msgLength = 0;
				}
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
						msgLength = 0;
						status = 0;
					}
	            }
	        } else if (res == 0) {
				waitFlag = TRUE;
			}
	   	}
    }
    return NULL;
}

int sendIFrame(int fd, char *frame, int size){
	char *msg;
	int msgSize, res;
	while (STOP == FALSE && lLayer.numTimeouts < lLayer.numTransmissions) {
		tcflush(fd, TCIOFLUSH);
		res = write(fd, frame, size);

		alarm(lLayer.timeout);

		msg = readMessage(fd, &msgSize);
		if(msg != NULL){
			if((unsigned char)analyse(msg) == (RR | lLayer.nr)){
				lLayer.nr = lLayer.nr^NR;
				lLayer.ns = lLayer.ns^NS;
				numI++;
				resetTimer();
				free(msg);
				printf("Sent %d bytes to serial port. (%d%%)\n", res, (total* 100)/fileSize);
				return 0;
			}
			else if((unsigned char)analyse(msg) == (REJ | lLayer.nr)){
				numRej++;
			}
		}
	}
	return -1;
}

char *createIFrame(int fd, int type, int *frameSize){ //adiciona flags, chama makeplayload

	char *frame = malloc(sizeof(char) * 20);
	char *packet, *stuffed, *stuffedBCC;
	int length = 7, stuffedLength, stuffedBCCLength;
	char bcc;

	frame[0] = FLAG;
	frame[1] = ADDRESS_SEND;
	frame[2] = lLayer.ns;
	frame[3] = frame[1]^frame[2];

	switch (type) {
		case START:
		case END:
			packet = createControlPacket(type, &length);	//contem ja bcc
			break;

		case DATA:
			length = lLayer.maxFrames + 4;
			packet = createDataPacket();
			break;
	}

	stuffed = byteStuffing(packet, length, &stuffedLength);
	bcc = getBCC2(packet, length);

	if(20 < (7+stuffedLength)){
		frame = realloc(frame, 7+stuffedLength);
	}
	memcpy(&frame[4], stuffed, stuffedLength);

	stuffedBCC = byteStuffing(&bcc, 1, &stuffedBCCLength);
	memcpy(&frame[4 + stuffedLength], stuffedBCC, stuffedBCCLength);
	length = stuffedLength + stuffedBCCLength;
	frame[4+length] = FLAG;

	free(packet); free(stuffed); free(stuffedBCC);
	(*frameSize) = (5+length);

	return frame;
}

int llread(char *msg, int msgLength){
	int destuffedLength;
	char *dataDestuffed = byteDestuffing(msg, msgLength, &destuffedLength);
	int type = dataDestuffed[0];

	if(msg[2] != lLayer.ns){
		printf("Non sequential packet! NS = %d\n", lLayer.ns);
		free(dataDestuffed);
		return -1;
	}

	if(checkBCC2(dataDestuffed, destuffedLength) == FALSE){
		printf("Error confirming BCC2.\n");
		free(dataDestuffed);
		return -1;
	}

	if(analyseIFrameData(dataDestuffed, type) < 0){
		free(dataDestuffed);
		return -1;
	}

	free(dataDestuffed);
	return 0;
}

int llwrite(int fd){

	STOP = FALSE;
	char *frame;
	int type = START, size, res;

	while(STOP == FALSE && lLayer.numTimeouts < lLayer.numTransmissions){
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
					close(appLayer.fd);
				}
				break;
		}
		free(frame);
	}
	if(STOP == FALSE) return -1;
	return 0;
}

int llopen(int fd){

	int size = CONTROL_PCK_SIZE;
	if(appLayer.transmission == TRANSMITTER){

		STOP = FALSE;
	    while (STOP == FALSE && lLayer.numTimeouts < lLayer.numTransmissions) {
	        sendSUFrame(fd, SET);
	        alarm(lLayer.timeout);

	        char *msg = readMessage(fd, &size);
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
		sendSUFrame(fd, UA);
		printf("Setup aknowlegded. Preparing for transfer\n");
		state = TRANSFERING;
		return 0;
	}
    return -1;
}

/*  Envia DISC e espera por UA ou por DISC dependendo se emite ou recebe*/
int llclose(int fd){
	char *msg;
	STOP = FALSE;
	int size = 20;
    if(appLayer.transmission == TRANSMITTER){

	    while(STOP == FALSE && lLayer.numTimeouts < lLayer.numTransmissions){

		    sendSUFrame(fd, DISC);
	        alarm(lLayer.timeout);
	        msg = readMessage(fd, &size);
	        if(analyse(msg) == DISC){
	        	printf("Ended transmission\n");
	            sendSUFrame(fd, UA);
	            STOP = TRUE;
	        }
			free(msg);
   		}
    } else if(appLayer.transmission == RECEIVER){

        while(STOP == FALSE && lLayer.numTimeouts < lLayer.numTransmissions){
		    sendSUFrame(fd, DISC);
	        alarm(lLayer.timeout);
	        msg = readMessage(fd, &size);

	        if(analyse(msg) == UA){
	            STOP = TRUE;
				printf("Confirmed transmission termination\n" );
	        }
			free(msg);
		}
    }
	if(STOP == FALSE) return -1;
    resetTimer();
    return 0;
}

/*  Envio de informacao do lado do emissor*/
int transfer(int fd){

    //int fd = open(fileName, O_RDONLY);

	while(state != TERMINATE){
	    switch(state){
			case BEGIN:
				if(llopen(fd)==0){
					state = TRANSFERING;
					printf("Setting up\n");
				} else {
					state = TERMINATE;
					printf("Error setting up. Shutting down\n");
				}
				break;
			case TRANSFERING:
				if(llwrite(fd) == -1){
					printf("Error during transfer. Shutting down\n");
					state = TERMINATE;

				} else{
					printf("Transfered the file successfully\n");
					state = DISCONNECT;
				}
				break;
			case DISCONNECT:
				if(llclose(fd) == -1){
					printf("Error while disconnecting\n");
				} else{
					printf("Disconnected successfully\n");
				}
				state = TERMINATE;
				break;

			case TERMINATE:
				break;
		}
	}

	printf("Read total of %d from file: %s\n", total, fileName);
	printf("Sent %d I frames and received %d rejections. Timedout %d times\n", numI, numRej, numTimeout);

    return 0;
}

/* Rececao do lado do recetor */
int receiver(int fd){
	int size = 70, res;
	char t;
	while(state != TERMINATE){
		char *msg = readMessage(fd,&size);
		switch(state){
			case BEGIN:
				if(analyse(msg) == SET){
					res = llopen(fd);
				} else{
					sendSUFrame(fd, REJ | lLayer.nr);
				}
				break;
			case TRANSFERING:

				if((t=(unsigned char)analyse(msg)) == lLayer.ns){	//control for I frames
					//lLayer.ns = lLayer.ns ^ NS;
					res = llread(msg, size);
					if(res == 0){
						sendSUFrame(fd, RR | lLayer.nr);
						lLayer.ns = lLayer.ns ^ NS;
						lLayer.nr = lLayer.nr^NR;
						numI++;
					} else {
						sendSUFrame(fd, REJ | lLayer.nr);
						numRej++;
					}

				} else if (t == DISC){
					res = llclose(fd);
					state = BEGIN;
					resetStatistics();
				} else if (t == SET){
					llopen(fd);
					lLayer.ns = 0; lLayer.nr = 0; total = 0; total = 0; free(fileName);
					resetStatistics();
				} else{
					sendSUFrame(fd, REJ | lLayer.nr);
					numRej++;
				}
				break;
			case DISCONNECT:
				if(analyse(msg) == DISC){
					res = llclose(fd);
					state = TERMINATE;
				} else{
					sendSUFrame(fd, REJ | lLayer.nr);
				}
				break;

			case TERMINATE:
				break;
		}
		free(msg);
	}
	printf("Wrote total of %d to file\n", total);
	printf("Received %d frames and %d rejects. Timedout %d times\n", numI, numRej, numTimeout);
    return 0;
}

/*
  ===================================================
*/

void defaultSettings(){
	lLayer.numTransmissions = 3;
	lLayer.timeout = 3;
	lLayer.baudRate = 0xB38400;
	lLayer.maxFrames = 350;
}

void manualSettings(char *baudRate, char *btf, char *retransmissions, char *timeout){
	char *end;
	lLayer.baudRate = checkBaudRate(strtoul(baudRate, &end, 10));
	if(*end != '\0' || lLayer.baudRate > 0x8000){
		printf("invalid value for BaudRate.\n");
		exit(1);
	}

	lLayer.maxFrames = strtoul(btf, &end, 10);
	if(*end != '\0' || lLayer.maxFrames > 0x8000){
		printf("invalid value for maxFrames.\n");
		exit(1);
	}
	lLayer.numTransmissions = strtoul(retransmissions, &end, 10);
	if(*end != '\0' || lLayer.numTransmissions > 0x8000){
		printf("invalid value for numTransmissions.\n");
		exit(1);
	}
	lLayer.timeout = strtol(timeout, &end, 10);
	if(*end != '\0' || lLayer.timeout > 0x8000){
		printf("invalid value for timeout.\n");
		exit(1);
	}

	printf("baudRate: %x\n", lLayer.baudRate);
	printf("maxFrames: %d\n", lLayer.maxFrames);
	printf("transmission: %d\n", lLayer.numTransmissions);
	printf("timeout: %d\n", lLayer.timeout);

}

void checkArguments(int argc, char **argv){
	if( (argc < 3) ||
		 ((strcmp("/dev/ttyS0", argv[1])!=0) &&
		  (strcmp("/dev/ttyS1", argv[1])!=0) )) {
			  printf("Default Usage:\tnserial SerialPort ConnectionMode (FileName) \n\tex: nserial /dev/ttyS1 TRANSMITTER pinguim.gif\n\tex: nserial /dev/ttyS1 RECEIVER\n");
				printf("Manual Usage:\tnserial SerialPort ConnectionMode (Filename) BaudRate (BytesPerFrame) (NumTransmissions) (Timeout)\n\tex: nserial /dev/ttyS1 TRANSMITTER pinguim.gif 38400 250 3 3\n\tex: nserial /dev/ttyS1 RECEIVER 38400\n");
			exit(1);
	} else{
		sprintf(lLayer.port, "%s", argv[1]);
	}

	if(strcmp("TRANSMITTER", argv[2])==0){
		appLayer.transmission = TRANSMITTER;

		if(argc == 4){
			if(checkFile(argv[3]) == -1){
				exit(1);
			}

			fileName = malloc(sizeof(char) * (strlen(argv[3])+1));
			sprintf(fileName, "%s", argv[3]);
			appLayer.fd = open(fileName, O_RDONLY);
			if(appLayer.fd < 0){
				printf("Error opening file\n");
				exit(1);
			}
			defaultSettings();
		} else if(argc == 8){
			if(checkFile(argv[3]) == -1){
				exit(1);
			}
			manualSettings(argv[4], argv[5], argv[6], argv[7]);
		} else{
			printf("Wrong number of arguments for %s\n", argv[2]);
			exit(1);
		}

	} else if(strcmp("RECEIVER", argv[2])==0){

		appLayer.transmission = RECEIVER;
		if(argc == 3){
			defaultSettings();
		} else if(argc == 4){
			char *end;
			lLayer.baudRate = checkBaudRate(strtoul(argv[3], &end, 10));
			if(*end != '\0' || lLayer.baudRate < 0){
				printf("invalid value for BaudRate.\n");
				exit(1);
			}
		} else{
			printf("Wrong number of arguments for %s\n", argv[2]);
			exit(1);
		}
	} else{
		printf("ConnectionMode not recognized\n\tuse: TRANSMITTER||RECEIVER\n");
		exit(1);
	}

	appLayer.packetSeq = -1;
	lLayer.ns = 0x00;
	lLayer.nr = 0x00;
}

void configSignals(int fd){
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
	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, FASYNC);
}

int main(int argc, char** argv) {

	struct termios oldtio,newtio;
	checkArguments(argc, argv);
    int fd = open(lLayer.port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  	*/
    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

	configSignals(fd);
    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    printf("New termios structure set\n");
	resetTimer();

	if(appLayer.transmission == TRANSMITTER){
		transfer(fd);
	} else{
		receiver(fd);
	}

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	free(fileName);

	close(fd);
    return 0;
}
