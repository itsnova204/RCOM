#include <stdint.h>
#include <stdio.h>

#include "packet.h"

uint8_t STOP_FLAG = 0x00;
uint8_t MODE;

uint8_t* status;

// Function to handle the Address byte
void handleAddress(uint8_t byte) {
    packet.address = byte;    
    currentState = STATE_CONTROL;
}

// Function to handle the Control byte
void handleControl(uint8_t byte) {
    packet.control = byte;
    handleFrameType();
}

// Function to handle the Frame Type (Information vs. Control)
void handleFrameType() {
    // If control field is 0x03 or 0x07, it's an Information frame
    if (packet.control == 0x03 || packet.control == 0x07) {
        packet.infoFrame.sequenceNumber = packet.control;
        packet.infoFrame.dataSize = 0;
        currentState = STATE_BCC1;
    } else {
        currentState = STATE_BCC1;
    }
}

// Function to handle Data bytes (for Information frames)
void handleData(uint8_t byte) {
    packet.infoFrame.data[packet.infoFrame.dataSize++] = byte;
    if (packet.infoFrame.dataSize >= MAX_DATA_SIZE) {
        currentState = STATE_BCC1;
    }
}

// Function to handle BCC1 byte
void handleBCC1(uint8_t byte) {
    packet.bcc1 = byte;

    if (packet.control == 0x00 || packet.control == 0x80) {//TODO: note this is a guess, powerpoint is not clear
         currentState = STATE_DATA;
    } else {
        currentState = STATE_END;
    }
}

// Function to handle BCC2 byte (for Information frames)
void handleBCC2(uint8_t byte) {
    packet.bcc2 = byte;
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

// Test driver
int main() {
    // Simulating the reception of a frame byte by byte
    uint8_t frame[] = {FLAG, 0x03, 0x00, 'H', 'e', 'l', 'l', 'o', 0x0F, 0xAA, FLAG};
    for (int i = 0; i < sizeof(frame); i++) {
        stateMachine(frame[i]);
    }
    return 0;
}
