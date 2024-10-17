#include <stdint.h>
#include <stdio.h>

#include "packet.h"

// Global variables
Frame frame;
State currentState = STATE_IDLE;
int byteIndex = 0;

// Function to handle the Address byte
void handleAddress(uint8_t byte) {
    frame.address = byte;    
    currentState = STATE_CONTROL;
}

// Function to handle the Control byte
void handleControl(uint8_t byte) {
    frame.control = byte;
    handleFrameType();
}

// Function to handle the Frame Type (Information vs. Control)
void handleFrameType() {
    // If control field is 0x03 or 0x07, it's an Information frame
    if (frame.control == 0x03 || frame.control == 0x07) {
        frame.infoFrame.sequenceNumber = frame.control;
        frame.infoFrame.dataSize = 0;
        currentState = STATE_BCC1;
    } else {
        currentState = STATE_BCC1;
    }
}

// Function to handle Data bytes (for Information frames)
void handleData(uint8_t byte) {
    frame.infoFrame.data[frame.infoFrame.dataSize++] = byte;
    if (frame.infoFrame.dataSize >= MAX_DATA_SIZE) {
        currentState = STATE_BCC1;
    }
}

// Function to handle BCC1 byte
void handleBCC1(uint8_t byte) {
    frame.bcc1 = byte;

    if (frame.control == 0x00 || frame.control == 0x80) {//TODO: note this is a guess, powerpoint is not clear
         currentState = STATE_DATA;
    } else {
        currentState = STATE_END;
    }
}

// Function to handle BCC2 byte (for Information frames)
void handleBCC2(uint8_t byte) {
    frame.bcc2 = byte;
    currentState = STATE_END;
}

// Function to handle End Flag
void handleEnd(uint8_t byte) {
    if (byte == FLAG) {
        printf("Frame received successfully!\n");
    }
    currentState = STATE_IDLE; // Reset to idle
}

// State machine handler
void stateMachine(uint8_t byte) {
    switch (currentState) {
        case STATE_IDLE:
            if (byte == FLAG) {
                currentState = STATE_ADDRESS;
            }
            break;
        case STATE_ADDRESS:
            handleAddress(byte);
            break;
        case STATE_CONTROL:
            handleControl(byte);
            break;
        case STATE_FRAME_TYPE:
            handleFrameType(byte);
            break;
        case STATE_DATA:
            handleData(byte);
            break;
        case STATE_BCC1:
            handleBCC1(byte);
            break;
        case STATE_BCC2:
            handleBCC2(byte);
            break;
        case STATE_END:
                (byte);
            break;
        default:
            currentState = STATE_IDLE;
            break;
    }
}

Frame createControlFrame(uint8_t type, uint8_t role) {
    Frame frame;
    //role = 0 = tx
    //is RR considered a reply to a I frame?
    if (role == 0){
        frame.address = 0x03;
    }else{
        frame.address = 0x01;
    }

    frame.control = type;
    frame.bcc1 = frame.address ^ frame.control;
    
    return frame;
}
    

int packetWrite(const unsigned char *buf, int bufSize) {
    
    return 0;
}
