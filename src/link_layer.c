// Link layer protocol implementation
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

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

    number_of_timeouts++;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters){   
    (void)signal(SIGALRM, alarmHandler);
    int alarmCount = 0;

    role = connectionParameters.role;
    printf("Opening connection as %d\n", role);
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
                    printf("Error writing frame WRITE()\n");
                    return -1;
                }
                printf("SET frame sent\n");
            }

            if (read_frame(&frameUA) < 0){
                printf("Error reading frame UA\n");
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
                continue;
            }

            if(frame.control == SET){
                printf("################# SET frame received ######################\n");

                Frame frameUA = {0};
                frameUA = create_frame(UA, RX_ADDR, NULL, 0);
                if (write_frame(frameUA) <= 0){
                    printf("Error writing frame\n");
                    alarmEnabled = FALSE;
                    return -1;
                }
                printf("################# UA frame sent ######################\n");
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
int llwritecalls = 0;

//return -1 big error
//return -1 timeout
//return -11 recived SET
//return -12 recived UA
//return -13 recived DISC
int llwrite(const unsigned char *buf, int bufSize) {
    printf("################# sending I frame %d ######################\n", llwritecalls++);
    number_of_frames++;
    
    uint8_t control = frame_number == 0 ? 0x00 : 0x80;
    latestFrame = create_frame(control, TX_ADDR, buf, bufSize);

    int bytesWritten = 0;
    Frame ackFrame = {0};
    int alarmCount = 0;

    int retransmit = 0;
    while (alarmCount <= retries) {
        // Send the frame
        bytesWritten = write_frame(latestFrame);

        if (bytesWritten < 0) {
            printf("Error writing to serial port\n");
            return -1;  // Serial port write error
        }

        // Set the alarm for timeout
        alarm(timeout);
        alarmEnabled = TRUE;
        retransmit = 0;

        // Wait for acknowledgment
        while (alarmEnabled) {
            int readout = read_frame(&ackFrame);

            if (readout < 0) {
                printf("Error reading frame BCC\n");
                continue;  

            } else if (readout == 0) {
                continue;  
            }

            if (readout == 1) {
                continue;  
            }

            if (ackFrame.control == (frame_number == 0 ? RR1 : RR0)) {
                // Correct ACK received, advance to the next frame
                frame_number = 1 - frame_number;  // Toggle between 0 and 1
                return bytesWritten;
            } else if (ackFrame.control == (frame_number == 0 ? REJ1 : REJ0)) {
                // Wrong ACK received, advance to the next frame
                printf("REJ received, accepting tho\n");
                frame_number = 1 - frame_number;  // Toggle between 0 and 1
                return bytesWritten;
            }else if (ackFrame.control == (frame_number == 0 ? REJ0 : REJ1)) {
                // REJ received, retransmit immediately
                printf("REJ received 1, retransmitting\n");
                number_of_retransmissions++;
                retransmit = 1;
                break;  // Exit inner loop to retransmit without counting as a retry
            } else if (ackFrame.control == (frame_number == 0 ? REJ1 : REJ0)){
                // REJ received, retransmit immediately
                frame_number = 1 - frame_number;
                printf("REJ received 2, retransmitting\n");
                number_of_retransmissions++;
                retransmit = 1;
                break;  // Exit inner loop to retransmit without counting as a retry
            }
            
        }

        if(retransmit == 1){
            continue;
        }
        // Timeout occurred, increment retry count
        alarmCount++;
        alarmEnabled = FALSE;  // Reset alarm flag for the next loop

        printf("Timeout waiting for ACK, retransmitting...\n");
    }

    printf("Maximum retries reached. Transmission failed.\n");
    return -1;  // This line only executes if retries exceed the limit
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
//return 0 if it read nothing aka error
//return -1 if it read a SET frame
//return >0 if it read a frame
int llreadcalls = 0;
int llread(unsigned char *packet) {
    number_of_frames++;
    printf("################# Reading I frame %d ######################\n", llreadcalls++);
    Frame frame = {0};
    int alarmCount = 0;
    int alarmEnabled = FALSE;

    alarm(timeout); 
    alarmEnabled = TRUE;

    int readOutput = 0;
    while (alarmCount <= retries) {
        if (alarmEnabled == FALSE) {
            alarm(timeout); 
            alarmEnabled = TRUE;
            printf("Waiting for frame TIMEOUT %d\n", alarmCount);
            alarmCount++;
        }

        readOutput = read_frame(&frame);

        if (readOutput == 0) { // No frame received, retry
            continue;
        }

        if(readOutput == -1){
            printf("Wrong bcc rejcting\n");
            uint8_t rej = frame_number == 0 ? REJ0 : REJ1;
            Frame frameREJ = create_frame(rej, RX_ADDR, NULL, 0);
            number_of_retransmissions++;

            if (write_frame(frameREJ) <= 0) {
                printf("Error writing REJ frame Wrong fn\n");
                return -1;
            }
            alarmEnabled = FALSE;
            alarmCount = 0;
            continue;
        }


        if (readOutput == 1) { // Frame not complete, keep reading
            continue;
        }

        // Address verification
        if (frame.address != TX_ADDR) { 
            printf("Address mismatch\n");
            uint8_t rej = frame_number == 0 ? REJ0 : REJ1;
            Frame frameREJ = create_frame(rej, RX_ADDR, NULL, 0);
            number_of_retransmissions++;
            if (write_frame(frameREJ) <= 0) {
                printf("Error writing REJ frame ADDR\n");
                return -1;
            }
            continue;
        }

        uint8_t expected_control = frame_number == 0 ? 0x00 : 0x80;
        if (frame.control == 0x00 || frame.control == 0x80) {
            if (frame.control == expected_control) { // SUCCESS: Correct frame received
                memcpy(packet, frame.infoFrame.data, frame.infoFrame.dataSize);
                
                // Send RR (Ready to Receive)
                frame_number = frame_number == 0 ? 1 : 0;
                uint8_t rr = frame_number == 0 ? RR0 : RR1;

                printf("Received frame %d, sending RR %02x\n", frame_number, rr);
                Frame frameRR = create_frame(rr, RX_ADDR, NULL, 0);
                if (write_frame(frameRR) <= 0) {
                    printf("Error writing RR frame\n");
                    return -1;
                }
                return frame.infoFrame.dataSize;  // Success, return data size
            } else {
                memcpy(packet, frame.infoFrame.data, frame.infoFrame.dataSize);
                
                // Send RR (Ready to Receive)
                uint8_t rr = frame_number == 0 ? RR0 : RR1;

                printf("Received frame %d, sending RR %02x\n", frame_number, rr);
                Frame frameRR = create_frame(rr, RX_ADDR, NULL, 0);
                if (write_frame(frameRR) <= 0) {
                    printf("Error writing RR frame\n");
                    return -1;
                }
                continue;
            }
        } else {
            // Handle special control frames
            switch (frame.control) {
                case SET: // Received SET frame in data communication
                    printf("SET frame received unexpectedly! Aborting and resetting.\n");
                    Frame frameUA = create_frame(UA, RX_ADDR, NULL, 0);
                    if (write_frame(frameUA) != 0) {
                        printf("Error writing UA frame\n");
                        return -1;
                    }
                    return -11;  // Aborted by SET

                case UA: // Unexpected UA frame, ignore
                    printf("UA frame received unexpectedly! Ignoring.\n");
                    return -12;

                case DISC: // Disconnect frame, end connection
                    printf("DISC frame received! Initiating disconnect sequence.\n");
                    while (alarmCount <= retries) {
                        if (alarmEnabled == FALSE) {
                            alarm(timeout); 
                            alarmEnabled = TRUE;
                            alarmCount++;

                            Frame frameDISC = create_frame(DISC, RX_ADDR, NULL, 0);
                            if (write_frame(frameDISC) != 0) {
                                printf("Error writing DISC frame\n");
                                return -1;
                            }
                        }

                        Frame frameUAdisc = {0};
                        if (read_frame(&frameUAdisc) != 0) { 
                            printf("Error reading DISC response\n");
                            return -1;
                        }

                        if (frameUAdisc.control == DISC && frameUAdisc.address == TX_ADDR) {
                            int clstat = closeSerialPort();
                            if (clstat != -1) {
                                return -13;  // Successful disconnect
                            } else {
                                return -1;  // Error in disconnecting
                            }
                        }
                    }
                    return -13;  // Disconnect failed
                default:
                    printf("Unknown control frame received: %x\n", frame.control);
                    break;
            }
        }
    }
    printf("Maximum retries reached. Timeout.\n");
    return -1;  // Timeout or max retries reached
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
//TODO implement statistics
int llclose(int showStatistics) {
    alarmEnabled = FALSE;
    alarmCount = 0;
    printf("Initiating disconnect sequence as %d\n", role);

    if (role == 116) {
        Frame frameDISC = {0};

        while (alarmCount <= retries) {

            if (alarmEnabled == FALSE) {
                alarm(timeout);
                alarmEnabled = TRUE;
                alarmCount++;

                // Send DISC to receiver
                printf("Sending DISC frame\n");
                Frame frame = create_frame(DISC, TX_ADDR, NULL, 0);
                if (write_frame(frame) < 0) {
                    printf("Error writing DISC frame\n");
                    return -1;
                }
            }

            int readout = read_frame(&frameDISC);

            if (readout == 0 || readout == 1) {
                continue;
            }

            if (readout < 0) {
                printf("Error bcc on disc\n");
                continue;
            }

            if (readout > 1) { 
                if (frameDISC.control == DISC && frameDISC.address == RX_ADDR) {
                    // Received DISC from receiver, send UA as acknowledgment
                    printf("Received DISC frame, sending UA\n");
                    Frame frameUA = create_frame(UA, TX_ADDR, NULL, 0);
                    
                    if (write_frame(frameUA) < 0) {
                        printf("Error writing UA frame\n");
                        return -1;
                    }
                    

                    printf("Connection closed successfully\n");
                    break;
                }
            } else if (read_frame(&frameDISC) < 0) {
                printf("Error reading frame\n");
                return -1;
            }

            // Reset alarmEnabled for next retry cycle
            alarmEnabled = FALSE;
        }
    }

    if (role == 114) {
        Frame frameDISC = {0};
        alarm(timeout);
        alarmEnabled = TRUE;
        while (alarmCount <= retries) {

            if (alarmEnabled == FALSE) {
                alarm(timeout);
                alarmEnabled = TRUE;
                alarmCount++;
                printf("Waiting for DISC frame\n");
            }

            int readout = read_frame(&frameDISC);

            if (readout == 0 || readout == 1) {
                continue;
            }

            if (readout < 0) {
                printf("Error bcc on disc\n");
                continue;
            }

            if (readout > 1) { 
                if (frameDISC.control == DISC) {
                    // Received DISC from receiver, send UA as acknowledgment
                    printf("Received DISC frame, sending DISC\n");
                    Frame frameDISC = create_frame(DISC, RX_ADDR, NULL, 0);
                    if (write_frame(frameDISC) < 0) {
                        printf("Error writing DISC frame\n");
                        return -1;
                    }
                    
                    
                }
                if (frameDISC.control == UA) {
                    printf("Connection closed successfully\n");
                    break;
                }
            } else if (read_frame(&frameDISC) < 0) {
                printf("Error reading frame\n");
                return -1;
            }

        }
    }

    // Print statistics if required
    if (showStatistics) {
        printf("Number of frames: %d\n", number_of_frames);
        printf("Number of retransmissions: %d\n", number_of_retransmissions);
        printf("Number of timeouts: %d\n", number_of_timeouts);
    }

    
    return closeSerialPort();
}
