// Application layer protocol implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "application_layer.h"
#include "link_layer.h"

#define ESC_OCTET 0x7d
#define FLAG 0x7e
#define XOR_OCTET 0x20

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    FILE* fptr;
    fptr = fopen(filename, "rb");

    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.role = *role;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if(llopen(connectionParameters) < 0){
        printf("Couldnt open the connection\n");
        return;
    }

    // unsigned char buffer[10];
    // fread(buffer,sizeof(buffer),1,fptr);
    // for(int i = 0; i<10; i++)
    //     printf("%u ", buffer[i]);
    
    fseek(fptr, 0, SEEK_END);
    long file_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    printf("File size: %ld bytes\n", file_size);
    
    int buffer_size = ceil(file_size / 100);

    printf("buffer size: %d bytes\n", buffer_size);

    unsigned char* buffer = (unsigned char*)malloc(buffer_size * sizeof(unsigned char));
    unsigned char tmp;

    while(fread(buffer,1,  buffer_size, fptr) > 0){
        for(int i=0; i<buffer_size; i++){
            //byte stuffing
            if(buffer[i] == FLAG || buffer[i] == ESC_OCTET){
                //increase size of buffer if necessary
                tmp = buffer[i] ^ XOR_OCTET;
                buffer_size += 1;
                buffer = (unsigned char*)realloc(buffer, buffer_size*sizeof(unsigned char));
                //shift elements
                for(int j=buffer_size-1; j>i;j--){
                    buffer[j] = buffer[j-1];
                }
                //replace
                buffer[i] = ESC_OCTET;
                buffer[i+1] = tmp;
            }
        }
        // stuff buffer
        // add packet header
        // llwrite(const unsigned char *buf, int bufSize)
    }   

    /*
    At the application layer
    » the file to be transmitted is fragmented - the fragments are encapsulated
        in data packets which are passed to the link layer one by one
    » in addition to data packets (which contain file fragments), the
        Application protocol uses control packets
    » the format of the packets (data and control) is defined ahead (slide 27 )
    */

    //Tx
    //form packets data or control
    //run llwrite(packet)

    //Tx
    //run info = llread()
    //unpack info

    //llclose
}
