// Application layer protocol implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    FILE* fptr;
    fptr = fopen(filename, "r");

    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.role = role;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if(llopen(connectionParameters) < 0){
        printf("Couldnt open the connection\n");
        return -1;
    }

    //Tx
    //form packets data or control
    //run llwrite(packet)

    //Tx
    //run info = llread()
    //unpack info

    //llclose
}
