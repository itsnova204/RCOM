// Application layer protocol implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "application_layer.h"
#include "link_layer.h"

#define MAX_PACKET_SIZE 2037 //1+1+2+256*8+255
#define TRUE 1
#define FALSE 0

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

int parsePacket(unsigned char *packet, int size, FILE* fptr)
{
    switch (packet[0])
    {
    case 1:
        //get attrs
        return 1;
    case 2:
        int Ls = 256*packet[2]+packet[3];
        for(int i=0;i<Ls+4;i++){
            printf("%x",packet[i]);
        }
        printf("\n");
        fwrite(&packet[4],1,Ls,fptr);
        return 2;
    case 3:
        //get & check attrs
        return 3;
    default:
        return -1;
    }
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *file_name)
{
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
    if(strcmp(role, "tx") == 0) connectionParameters.role = LlTx;
    else connectionParameters.role = LlRx;
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if(llopen(connectionParameters) < 0){
            printf("Couldnt open the connection\n");
            exit(-1);
        }

    if(strcmp(role,"tx") == 0){
        FILE* fptr;
        fptr = fopen(file_name, "rb");
    
        fseek(fptr, 0, SEEK_END);
        long file_size = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);

        printf("File size: %ld bytes\n", file_size);
        
        int buffer_size = ceil(file_size / 100);

        printf("buffer size: %d bytes\n", buffer_size);

        unsigned char* buffer = (unsigned char*)malloc(buffer_size * sizeof(unsigned char));

        Result r = getControlPacket(1, file_name, file_size);
        
        printf("Sending control packet [1]\n"); \
        
        //TODO handle return value of llwrite
        llwrite(r.pointer, r.value); 
        
        FILE *ftest;    
        ftest = fopen("penguin-rec-test.gif", "wb");


        //TODO actually read a file
        int sequence = 0;
        int actual_size;
        while((actual_size = fread(buffer,1,  buffer_size, fptr)) > 0){
            printf("act = %d, buf = %d\n",actual_size,buffer_size);
            r = getDataPacket(buffer, actual_size, sequence);

                        int Ls = 256*r.pointer[2]+r.pointer[3];
                        // for(int i=0;i<Ls+4;i++){
                        //     printf("%x",packet[i]);
                        // }
                        // printf("\n");
                        printf("test sequence #%d\n", sequence);
                        fwrite(&r.pointer[4],1,Ls,ftest);

            printf("sequence #%d sent\n", r.pointer[1]);
            for(int i=0;i<256*r.pointer[2]+r.pointer[3]+4;i++){
                printf("%x",r.pointer[i]);
            }
            printf("\n");
            llwrite(r.pointer, r.value);
            sequence++;
        }   

        r = getControlPacket(3, file_name, file_size);
        printf("Sending control packet [3]\n");
        llwrite(r.pointer, r.value);

        //TODO use llclose

        fclose(fptr);
        fclose(ftest);
    }
    else{
        FILE* fptr;
        fptr = fopen(file_name, "wb");
        int result;
        unsigned char* buffer = (unsigned char*)malloc(MAX_PACKET_SIZE * sizeof(unsigned char));

        int END = FALSE;

        //TODO handle control packets

        while(!END){
            result = llread(buffer);
            if(result>=0){
                if(parsePacket(buffer, result, fptr)==3) END = TRUE;
            }
        }
        fclose(fptr);
    }

    //llclose cant call it here! it must be before fclose
}
