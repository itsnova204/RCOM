#include <stdint.h>
#include <stdio.h>

#include "frame.h"
#include "serial_port.h"

//+----------+---------+---------+---------+----------+----------+----------+----------+
//|   Flag   | Address | Control | Length  |   BCC1   |   Data   |   BCC2   |   Flag   |
//+----------+---------+---------+----------+----------+----------+---------+----------+
//| 01111110 | 8 bits  | 8 bits  | 8 bits  |  8 bits  | 0-256*8  | 8 bits   | 01111110 |
//+----------+---------+---------+---------+----------+----------+----------+----------+


// Global variables
State currentState = STATE_IDLE;
int byteIndex = 0;

// Function to handle the Address byte
void handleAddress(uint8_t byte, Frame* frame) {
    if (byte == FLAG) {
        printf("FOUND FLAG IN ADDRESS! RESSETING!\n");
        return;
    }
    frame->address = byte;    
    currentState = STATE_CONTROL;
}

// Function to handle the Control byte
void handleControl(uint8_t byte, Frame* frame) {
    frame->control = byte;
}

// Function to handle BCC1 byte
void handleBCC1(uint8_t byte, Frame* frame) {
    frame->bcc1 = byte;

    if ((frame->control ^ frame->address) != frame->bcc1){
        //RECIVED INVALID HEADER DISCARTING FRAME!
        currentState = STATE_IDLE;
    }else{
        currentState = STATE_LENGTH;
    }
    
}

void handleLength(uint8_t byte, Frame* frame) { //length can be 0x7E
    if (byte == 0) currentState = STATE_END; //if length is 0, there is no data aka control frame.
    
    frame->infoFrame.stuffedDataSize = byte;
    currentState = STATE_DATA;
}

// Function to handle Data bytes (for Information frames)
void handleData(uint8_t byte, Frame* frame) {
    if (byte == FLAG) {
        printf("FOUND FLAG IN DATA! RESSETING!\n");
        currentState = STATE_ADDRESS; // Reset to idle
        return;
    }

    frame->infoFrame.data[byteIndex++] = byte;
    if (byteIndex == frame->infoFrame.stuffedDataSize) {
        currentState = STATE_BCC2;
    }
}

// Function to handle BCC2 byte (for Information frames)
void handleBCC2(uint8_t byte, Frame* frame) { //bcc2 can be 0x7E
    for (int i = 0; i < frame->infoFrame.stuffedDataSize; i++) {
        frame->infoFrame.bcc2 ^= frame->infoFrame.data[i];
    }

    if (frame->infoFrame.bcc2 == byte){
        currentState = STATE_END;
    }else{
        //RECIVED INVALID DATA DISCARTING FRAME!
        printf("Invalid BCC2\n");
        currentState = STATE_IDLE;
    }
}

// Function to handle End Flag
int handleEnd(uint8_t byte) {
    if (byte == FLAG) {
        printf("Frame received successfully!\n");
        currentState = STATE_IDLE; // Reset to idle
        return 0;
    }else{
        //RECIVED INVALID END FLAG DISCARTING FRAME!
        printf("Invalid End Flag\n");
    }
    return 1;
}

// State machine handler
void stateMachine(uint8_t byte, Frame* frame) {
    switch (currentState) {
        case STATE_IDLE:
            if (byte == FLAG) {
                currentState = STATE_ADDRESS;
            }
            break;
        case STATE_ADDRESS:
            handleAddress(byte, frame);
            break;
        case STATE_CONTROL:
            handleControl(byte, frame);
            break;
        case STATE_BCC1:
            handleBCC1(byte, frame);
            break;
        case STATE_LENGTH:
            handleLength(byte, frame);
            break;
        case STATE_DATA:
            handleData(byte, frame);
            break;
        case STATE_BCC2:
            handleBCC2(byte, frame);
            break;
        case STATE_END:
            handleEnd(byte);
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
    
int writeFrame(Frame frame) {
    char buffer[MAX_DATA_SIZE];
    int frameSize = 0;
    buffer[frameSize++] = FLAG;
    buffer[frameSize++] = frame.address;
    buffer[frameSize++] = frame.control;
    buffer[frameSize++] = frame.bcc1;

    //Add data if it's an I frame
    if (frame.control == 0x03 || frame.control == 0x07) {
        for (int j = 0; j < frame.infoFrame.stuffedDataSize; j++) {
            buffer[frameSize++] = frame.infoFrame.data[j];
        }
        buffer[frameSize++] = frame.infoFrame.bcc2;
    }

    buffer[frameSize++] = FLAG;
    const char *bytes = buffer;

    return writeBytes(bytes, frameSize);;
}

int readFrame(Frame* frame) { //TODO: Add alarm
    char byte;
    int readBytes = 0;
    int alarmCounter = 0; //TODO: Add alarm


    while (alarmCounter < 3) {
        readBytes = readByte(&byte);
        stateMachine(byte, frame);
        printf("Read %d bytes\n", readBytes);
    }

    return 0;
}

int readFrame(char* buffer) { //TODO: Add alarm
    int bufferIndex = 0; 
    char byte;
    int readBytes = 0;
    int alarmCounter = 0; //TODO: Add alarm


    while (alarmCounter < 3) {
        readBytes = readByte(&byte);

        if (byte == FLAG && bufferIndex < 4){ //Too small, cant never be valid frame
            bufferIndex = 0;
        }
        
        buffer[readBytes] = byte;
    }

    return 0;
}

int validateFrame(Frame frame){
    uint8_t bcc1 = frame.address ^ frame.control;
    if (bcc1 != frame.bcc1){ 
        return 1;
    }
    if (frame.control == 0x03 || frame.control == 0x07) {
        uint8_t bcc2 = 0;
        for (int i = 0; i < frame.infoFrame.stuffedDataSize; i++) {
            bcc2 ^= frame.infoFrame.data[i];
        }
        if (bcc2 != frame.infoFrame.bcc2){
            return 2;
        }
    }
    return 0;
}
