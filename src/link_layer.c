// Link layer protocol implementation
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "link_layer.h"
#include "serial_port.h"
#include "frame.h"


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define TX_ADDR 0x03
#define RX_ADDR 0x01

#define FALSE 0
#define TRUE 1


int alarmEnabled = FALSE;
int alarmCount = 0;

// Statistics
int number_of_frames = 0;
int number_of_retransmissions = 0;
int number_of_timeouts = 0;




// Global variables
LinkLayerRole role;

uint8_t frame_number = 0x00;
Frame latestFrame;
int timeout = 3;
int retries = 3;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("TIMEOUT %d\n", alarmCount);
    number_of_timeouts++;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){   
    (void)signal(SIGALRM, alarmHandler);
    int alarmCount = 0;

    role = connectionParameters.role;
    timeout = connectionParameters.timeout;
    retries = connectionParameters.nRetransmissions;

    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) return -1;

    if (role == LlTx){
        Frame frame = create_frame(SET, TX_ADDR, NULL, 0);
        Frame frameUA = {0};
        while(alarmCount <= retries){
            if (alarmEnabled == FALSE){
                alarm(timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
                alarmCount++;

                if(write_frame(frame) <= 0){
                    printf("Error writing frame\n");
                    return -1;
                }
                printf("SET frame sent\n");
            }

            if (read_frame(&frameUA) < 0){
                printf("Error reading frame\n");
                return -1;
            }

            if(frameUA.control == UA && frameUA.address == RX_ADDR){
                printf("################# UA frame received ######################\n");
                return fd;
            }
        }

        return -1;

    }else if (role == LlRx){
        Frame frame = {0};

        while(alarmCount <= retries){ 
            if (alarmEnabled == FALSE){
                alarm(timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
                alarmCount++;
            }

            int readout = read_frame(&frame);
            if(readout == 0){ //read nothing try again
                continue;
            }

            
            if(readout < 0){ 
                printf("Error reading frame\n");
                alarmEnabled = FALSE;
                return -1;
            }

            if(frame.control == SET){
                printf("SET frame received\n");

                Frame frameUA = {0};
                frameUA = create_frame(UA, RX_ADDR, NULL, 0);
                if (write_frame(frameUA) <= 0){
                    printf("Error writing frame\n");
                    alarmEnabled = FALSE;
                    return -1;
                }
                alarmEnabled = FALSE;
                return fd;
            }
        }
        alarmEnabled = FALSE;
        return fd;
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    latestFrame = create_frame(frame_number, TX_ADDR, buf, bufSize);
    
    int bytesWritten = 0;
    Frame ackFrame = {0};

    int alarmCount = 0;
    
    while (alarmCount <= retries) {
        if (alarmEnabled == FALSE){
            alarm(timeout); 
            alarmEnabled = TRUE;
            alarmCount++;
            
            // Send the data frame
            bytesWritten = write_frame(latestFrame);  // frameNumber = 0 or 1 in Stop-and-Wait
            if (bytesWritten < 0) {
                printf("Error writing to serial port");
                return -1;  // Serial port write error
            }
        }

        // Wait for acknowledgment (RR or REJ)
        if(read_frame(&ackFrame) != 0){
            printf("Error reading frame\n");
            return -1;
        }
        
        if (ackFrame.infoFrame.dataSize > 0){
            printf("recived data, expecting ack\n");
        }
        
        switch (ackFrame.control){
            case RR0:
                if (frame_number == 1) { //success
                    frame_number = 0x00;
                    return bytesWritten;
                }else{ //error probably timeout, retransmit
                    continue;
                }            
                break;

            case RR1:
                if (frame_number == 0) { //success
                    frame_number = 0x80;
                    return bytesWritten;
                }else{ //error probably timeout, retransmit
                    continue;
                }            
                break;

            case REJ0:
                if (frame_number == 0) { //error, retransmit
                    continue;
                }else{ //error, retransmit
                    continue;
                }            
                break;

            case REJ1:
                if (frame_number == 1) { //error, retransmit
                    continue;
                }else{ //error, retransmit
                    continue;
                }            
                break;

            case DISC: //received disconnect, close connection

                break;
            default:
                break;
            }
    }
    

    return -1; //timeout
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
//return 0 if it read nothing aka error
//return -1 if it read a SET frame
//return >0 if it read a frame
int llread(unsigned char *packet){
    Frame frame = {0};


    

    int alarmCount = 0;
    int alarmEnabled = FALSE;

    while (alarmCount <= retries){
         if (alarmEnabled == FALSE){
            alarm(timeout); 
            alarmEnabled = TRUE;
            printf("Waiting for frame TIMEOUT\n");
            alarmCount++;
        }

        int readOutput = read_frame(&frame);
        if(readOutput == 0){ //read nothing try again
            continue;
        }

        if (readOutput < 0){//bcc failed!
            Frame frameREJ = create_frame(frame_number, RX_ADDR, NULL, 0);

            if (write_frame(frameREJ) != 0){
                printf("Error writing frame\n");
                return 0;
            }
            return 0;
        }

        if (frame.address != TX_ADDR){ //failed address
            Frame frameREJ = create_frame(frame_number, RX_ADDR, NULL, 0);

            if (write_frame(frameREJ) <= 0){
                printf("Error writing frame\n");
                return 0;
            }
            return 0;
        }
        
        if(frame.control == 0x00 || frame.control == 0x80){
            if (frame.control == frame_number){
                //send RR
                frame_number = frame_number == 0 ? RR0 : RR1;

                Frame frameRR = create_frame(frame_number, RX_ADDR, NULL, 0);
                int write_out = write_frame(frameRR);
                if (write_out <= 0){
                    printf("Error writing frame\n");
                    return 0;
                }

                return write_out;
            }else{
                //send REJ
                Frame frameREJ = create_frame(frame_number, RX_ADDR, NULL, 0);

                if (write_frame(frameREJ) <= 0){
                    printf("Error writing frame\n");
                    return 0;
                }
                return 0;
            }
        }else{
            switch (frame.control)
            {
            case SET: //received SET frame
                printf("SET frame received in middle of transmition! ABORTING AND RESETING EVERYTHING\n");
                Frame frameUA = create_frame(UA, RX_ADDR, NULL, 0);
                if (write_frame(frameUA) != 0){
                    printf("Error writing frame\n");
                    return 0;
                }
                return -1;
                break;
            case UA:
                printf("UA frame received in middle of transmition! Ignoring\n");
                break;
            case DISC:
                printf("DISC frame received in middle of transmition! Stopping\n");
                return -2;
                break;
            
            default:
                break;
            }
        }
    }    
    return 0; //timeout
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
//TODO implement statistics
int llclose(int showStatistics) 
{
    int alarmCount = 0;
    int alarmEnabled = FALSE;

    if (role == LlTx){

        
        Frame frameDISC = {0};
        
        while (alarmCount <= retries){ //wait for DISC rx
            if (alarmEnabled == FALSE){
                alarm(timeout); 
                alarmEnabled = TRUE;
                alarmCount++;

                Frame frame = create_frame(DISC, TX_ADDR, NULL, 0);
                if (write_frame(frame) != 0){
                    printf("Error writing frame\n");
                    return -1;
                }
            }

            if(read_frame(&frameDISC) != 0){ 
                printf("Error reading frame\n");
                return -1;
            }

            if(frameDISC.control != DISC && frameDISC.control == RX_ADDR){
                Frame frameUA = create_frame(UA, TX_ADDR, NULL, 0);
                if (write_frame(frameUA) != 0){
                    printf("Error writing frame\n");
                    return -1;
                }
                int clstat = closeSerialPort();
                return clstat;
            }
        }


    }

    if (role == LlRx){

        Frame frameUA = {0};
        while (alarmCount <= retries) {
            if (alarmEnabled == FALSE){
                alarm(timeout); 
                alarmEnabled = TRUE;
                alarmCount++;
                
                Frame frameDISC = {0};
                if (write_frame(frameDISC) != 0){
                    printf("Error writing frame\n");
                    return -1;
                }
            }
            
            if(read_frame(&frameUA) != 0){ 
                printf("Error reading frame\n");
                return -1;
            }

            if(frameUA.control == DISC && frameUA.address == TX_ADDR){
                int clstat = closeSerialPort();
                return clstat;
            }
        }
        
    }

    return -1;
}
