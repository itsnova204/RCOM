// Link layer protocol implementation
#include <stdio.h>

#include "link_layer.h"
#include "serial_port.h"
#include "frame.h"


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }

    if (connectionParameters.role == LlTx)
    {
        Frame frame = createControlFrame(SET, LlTx);
        return writeFrame(frame);
    }
    else if (connectionParameters.role == LlRx)
    {
        Frame frame;
        readFrame(&frame);
        int frameStatus = validateFrame(frame);
        
        if (frameStatus == 1){   
            printf("Frame is invalid, bcc1 failed!\n");
            return 1;
        }

        if (frameStatus == 2){
            printf("Frame is invalid, bcc2 failed!\n");
            return 2;
        }
        //frame ok!

        Frame frameUA = createControlFrame(UA, LlRx);
        return writeFrame(frameUA);
    }
    

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
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
