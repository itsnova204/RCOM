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

typedef struct {
    int value;      // integer value
    unsigned char *pointer;  // pointer to a character array
} Result;

Result getControlPacket(int control_field, const char *file_name, long file_size)
{
    char C, T1, T2, L1, L2;
    C = control_field;
    T1 = 0; //file size
    T2 = 1; //file name
    L1 = 8; //long is 8 bytes
    L2 = strlen(file_name);

    int packet_length = 5+L1+L2; //5 FOR C, Ts and Ls
    int index = 0;

    unsigned char *packet = (unsigned char*) malloc((packet_length) * sizeof(char));
    
    packet[index++] = C;
    packet[index++] = T1;
    packet[index++] = L1;

    packet[index++] = (file_size >> 56) & 0xFF; // Extract the most significant byte
    packet[index++] = (file_size >> 48) & 0xFF; // Extract the second byte
    packet[index++] = (file_size >> 40) & 0xFF; // Extract the third byte
    packet[index++] = (file_size >> 32) & 0xFF; // Extract the fourth byte
    packet[index++] = (file_size >> 24) & 0xFF; // Extract the fifth byte
    packet[index++] = (file_size >> 16) & 0xFF; // Extract the sixth byte
    packet[index++] = (file_size >> 8) & 0xFF;  // Extract the seventh byte
    packet[index++] = file_size & 0xFF;

    packet[index++] = T2;
    packet[index++] = L2;

    memcpy(&packet[index], file_name, L2);  // Copy file name into packet
    index += L2;
    Result r;
    r.pointer = packet;
    r.value = packet_length;

    return r;
}

Result getDataPacket(unsigned char *buf, int buf_size, int sequence_number)
{
    unsigned char *packet = (unsigned char*) malloc((buf_size + 4) * sizeof(char));
    int index=0;
    packet[index++] = 2;
    packet[index++] = sequence_number;
    packet[index++] = (buf_size >> 8) & 0xFF;
    packet[index++] = buf_size & 0xFF;
    memcpy(&packet[index], buf, buf_size);
    Result r;
    r.pointer = packet;
    r.value = buf_size+4;

    return r;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *file_name)
{
    if(strcmp(role,"tx") == 0){
        FILE* fptr;
        fptr = fopen(file_name, "rb");

        LinkLayer connectionParameters;
        strcpy(connectionParameters.serialPort, serialPort);
        connectionParameters.role = *role;
        connectionParameters.baudRate = baudRate;
        connectionParameters.nRetransmissions = nTries;
        connectionParameters.timeout = timeout;

        if(llopen(connectionParameters) < 0){
            printf("Couldnt open the connection\n");
            exit(-1);
        }
        
        fseek(fptr, 0, SEEK_END);
        long file_size = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);

        printf("File size: %ld bytes\n", file_size);
        
        int buffer_size = ceil(file_size / 100);

        printf("buffer size: %d bytes\n", buffer_size);

        unsigned char* buffer = (unsigned char*)malloc(buffer_size * sizeof(unsigned char));

        Result r = getControlPacket(1, file_name, file_size);
        llwrite(r.pointer, r.value);

        int sequence = 0;
        while(fread(buffer,1,  buffer_size, fptr) > 0){
            r = getDataPacket(buffer, buffer_size, sequence);
            llwrite(r.pointer, r.value);
            sequence++;
        }   

        r = getControlPacket(3, file_name, file_size);
    }
    else{

    }
}
