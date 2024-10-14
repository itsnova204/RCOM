#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A 0x03
#define C 0x03
#define BCC 0x00  // A^C
#define BUF_SIZE 6

int alarmEnabled = FALSE;
int alarmCount = 0;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(LinkLayer *link){
    int fd = open(link->serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        // perror("tcgetattr");
        // exit(-1);
        return -1;
    }
    struct termios oldtio;
    struct termios newtio;
    if (tcgetattr(fd, &oldtio) == -1)
    {
        // perror("tcgetattr");
        // exit(-1);
        return -1;
    }
    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        // perror("tcsetattr");
        // exit(-1);
        return -1;
    }
    if(link->role == TRANSMITTER){
        printf("sending set frame...");
        unsigned char buf[BUF_SIZE] = {FLAG,A,C,BCC,FLAG,'\n'};
        (void)signal(SIGALRM, alarmHandler);
        int STOP = FALSE;
        int x;
        
        while (alarmCount < link->nRetransmissions && STOP == FALSE)
        {
            if (alarmEnabled == FALSE)
            {
                int bytes = write(fd, buf, BUF_SIZE);
                printf("%d bytes written\n", bytes);
                alarm(link->timeout);
                alarmEnabled = TRUE;
            }
            
            x = read(fd, buf, BUF_SIZE);            
            //printf("%02x\n", x);
            if(buf[0] == FLAG &&
                buf[1] == 0x01 &&
                buf[2] == 0x07 &&
                buf[3] == 0x06 &&
                buf[4] == FLAG)
            {
                STOP = TRUE;
                //printf("UA got\n");
            }
            
            
        }
        
        if(alarmCount >= link->nRetransmissions){
            printf("smh went wrong\n");
        }
        
        if(STOP){
            printf("%02x\n", buf[3]);
            printf("UA got\n");
        }
        
        sleep(1);

        // Restore the old port settings
        if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
        {
            perror("tcsetattr");
            exit(-1);
        }
    }
    else{
        printf("receiving set frame...");
    }

    return 0;
}

int llwrite(){

    return 0;
}

int llread(){

    return 0;
}

int llclose(){

    return 0;
}