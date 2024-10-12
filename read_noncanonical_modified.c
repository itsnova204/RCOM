// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400 
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FLAG    0x7E
#define ADDR_R  0x03
#define ADDR_S  0x01
#define CTRL_SET 0x03
#define CTRL_UA  0x07

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;

int handle_flag(char byte){
    if(byte == FLAG){
        return 0;
    }
    return 1;
}

int handle_address(char byte){
    if(byte == ADDR_R){
        return 0;
    }
    if(byte == ADDR_S){ //TODO: make it check its role
        return 1;
    }
    return 1;
}

int handle_control(char byte){ //TODO: make it handle more control bytes
    if(byte == CTRL_SET){
        return 0;
    }
    return 1;
}

int handle_BCC(char byte, char* bufferToCheck, int size){ //TODO: make it more robust, do not trust int size
    char xor = bufferToCheck[0];
    for (int i = 1; i < size; i++){
        xor = bufferToCheck[i] ^ xor;
    }

    if (byte == xor){
        return 0;
    }
    
    return 1;
}

enum state{
    waiting_flag1,
    waiting_address,
    waiting_control,
    waiting_BCC,
    waiting_flag2
};


enum state control_state = waiting_flag1;
int sizeOfRecievedBuffer = 0;
char recievedBuffer[BUF_SIZE] = {0};

int controlFrame_stateMachine(char byte){//TODO check if buf full

    recievedBuffer[sizeOfRecievedBuffer] = byte;
    sizeOfRecievedBuffer++;

    switch (control_state){
    case waiting_flag1:
        if(handle_flag(byte) == 0){
            control_state++;
            return 0;
        }
        break;
    case waiting_address:
        if(handle_address(byte) == 0){
            control_state++;
            return 0;
        }
        break;
    case waiting_control:
        if(handle_control(byte) == 0){
            control_state++;
            return 0;
        }
        break;
    case waiting_BCC:
        if(handle_BCC(byte, recievedBuffer, sizeOfRecievedBuffer) == 0){
            control_state++;
            return 0;
        }
        break;
    case waiting_flag2:
        if(handle_flag(byte) == 0){
            control_state = waiting_flag1;
            return 2;
        }
    break;

    default:
        break;
    }

    //if not returned earlier, something whent wrong. RESETTING
    sizeOfRecievedBuffer = 0;
    control_state = waiting_flag1;
    return 1;
}


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char
    
    int currentChar = 0;
    int readingFrame = 0;    

    //char bytes;
    while (STOP == FALSE){
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, 1);
        
        controlFrame_stateMachine(bytes);
    }
    
    buf[0] = FLAG;
    buf[1] = ADDR_S; 
    buf[2] = CTRL_UA; 
    buf[3] = buf[1] ^ buf[2];
    buf[4] = FLAG;
    
    
    int bytes = write(fd,buf,BUF_SIZE);
    printf("%d bytes written\n", bytes);

   
    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

