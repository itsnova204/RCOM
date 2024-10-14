#ifndef LINK_LAYER_H
#define LINK_LAYER_H

typedef enum {
    TRANSMITTER,
    RECEIVER
} role;

// struct from guide
typedef struct {
    char serialPort[50];  /*Device /dev/ttySx, x = 0, 1*/
    role role;            /*TRANSMITTER | RECEIVER*/
    int baudRate;         /*Speed of the transmission*/
    int nRetransmissions; /*Number of retries in case of failure*/
    int timeout;          /*Timer value: 1 s*/
} LinkLayer;

void alarmHandler(int signal);

int llopen(LinkLayer *link);
int llwrite();
int llread();
int llclose();

#endif