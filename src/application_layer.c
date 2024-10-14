#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "application_layer.h"
#include "link_layer.h"

#define DEFAULT_BAUDRATE 9600  // For the delaying transmissions
#define DEFAULT_NRETRANSMISSIONS 3
#define DEFAULT_TIMEOUT 2

#define BAUDRATE B38400

int transmitFile(char *file, char *serialPortName){
    LinkLayer link;

    strcpy(link.serialPort,serialPortName);
    link.role = TRANSMITTER;
    link.baudRate = DEFAULT_BAUDRATE;
    link.nRetransmissions = DEFAULT_NRETRANSMISSIONS;
    link.timeout = DEFAULT_TIMEOUT;

    llopen(&link);
    llwrite();
    llclose();
    return 0;
}

int receiveFile(char *file, char *port){
    LinkLayer link;

    strcpy(link.serialPort,port);
    link.role = RECEIVER;
    link.baudRate = DEFAULT_BAUDRATE;
    link.nRetransmissions = DEFAULT_NRETRANSMISSIONS;
    link.timeout = DEFAULT_TIMEOUT;

    llopen(&link);
    llread();
    llclose();
    return 0;
}