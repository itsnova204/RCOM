// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

#include <stdio.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN DON BY TIAGO
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        printf("here :)\n");
        return -1;
    }

    // TODO

    return 1;
}

////////////////////////////////////////////////
// LLWRITE DONE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int bytes = writeBytesSerialPort(buf,bufSize);
    //printf("%d bytes written, %d sequence ",bytes, buf[1]);

    unsigned char tmp[3];
    int Stop = 0;

    //wait(1);

    while(Stop == 0){
        readByteSerialPort(&tmp[0]);
        readByteSerialPort(&tmp[1]);
        readByteSerialPort(&tmp[2]);
        //printf("%d, %d, %d\n", tmp[0],tmp[1],tmp[2]);
        if(tmp[0] == 1 && tmp[1] == 2 && tmp[2] == 3)
        {
            Stop = 1;
        }
    }

    return 0;
}

////////////////////////////////////////////////
// LLREAD DONE
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int try=0;
    int index = 0;
    const unsigned char tmp[3] = {1,2,3};
    while(readByteSerialPort(&packet[index])<0){}
    index++;

    printf("packet[0] == %d\n",packet[0]);
    if(packet[0] == 1){
        printf("set packet got\n");
        while(readByteSerialPort(&packet[index])>0){
            printf(packet[index]);
            index++; //like flush??
        }
        writeBytesSerialPort(&tmp[0], 3);
        return 1;
    }
    if(packet[0] == 3){
        printf("ending packet got\n");
        while(readByteSerialPort(&packet[index])>0){
            printf(packet[index]);
            index++; //like flush??
        }
        writeBytesSerialPort(&tmp[0], 3);
        return 1;
    }
    for(index;index<4;index++){
        while(readByteSerialPort(&packet[index])<0){}
    }
    printf("sequence #%d received\n", packet[1]);
    for(long i=packet[2]*256+packet[3];i>0;i--){
        while(readByteSerialPort(&packet[index])<0){
            try++;
            if(try>3){
                break;
            }
        }
        //printf("%x - %d\n", packet[index], index);
        index++;
    }
    //printf("\n");
    
    writeBytesSerialPort(&tmp[0], 3);

    return 1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
