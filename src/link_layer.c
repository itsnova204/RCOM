// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define ESC_OCTET 0x7d
#define FLAG 0x7e
#define XOR_OCTET 0x20

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
    }
    else if (connectionParameters.role == LlRx)
    {
        /* code */
    }
    

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // stuffing function
    typedef struct{
        unsigned char *buffer;
        int buffer_size;
    } help_struct;

    help_struct stuff(unsigned_char *buffer, int buffer_size){
        // for every char except first and last (flags)
        for(int i=1; i<buffer_size-1; i++){
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
        help_struct r;
        r.buffer = buffer;
        r.buffer_size = buffer_size;
        return r;
    }
    

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
