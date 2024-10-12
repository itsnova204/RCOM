#ifndef LINK_LAYER_H
#define LINK_LAYER_H

enum role {
    TRANSMITTER,
    RECEIVER
};

// struct from guide
struct LinkLayer {
    char serialPort[50];  /*Device /dev/ttySx, x = 0, 1*/
    role role;            /*TRANSMITTER | RECEIVER*/
    int baudRate;         /*Speed of the transmission*/
    int nRetransmissions; /*Number of retries in case of failure*/
    int timeout;          /*Timer value: 1 s*/
};

int llopen();
int llwrite();
int llread();
int llclose();

#endif