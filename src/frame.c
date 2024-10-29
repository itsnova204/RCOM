#include <stdint.h>
#include <stdio.h>

#include "frame.h"
#include "serial_port.h"


// Global variables
#define BUFSIZE 1024


int bufferSize = 0; 
char buffer[BUFSIZE] = {0};

int processedBufferSize = 0;
char processedBuffer[BUFSIZE] = {0};
void stuffing() {
    processedBufferSize = 0;

    for (int i = 0; i < bufferSize; i++) {
        uint8_t byte = buffer[i];
        
        if (byte == FLAG) {
            // Replace 0x7E with 0x7D 0x5E
            processedBuffer[processedBufferSize++] = ESCAPE;
            processedBuffer[processedBufferSize++] = byte ^ 0x20;
        } else if (byte == ESCAPE) {
            // Replace 0x7D with 0x7D 0x5D
            processedBuffer[processedBufferSize++] = ESCAPE;
            processedBuffer[processedBufferSize++] = byte ^ 0x20;
        } else {
            // Regular byte, add to output
            processedBuffer[processedBufferSize++] = byte;
        }
    }
}

void destuffing() {
    processedBufferSize = 0;
    for (int i = 0; i < bufferSize; i++) {
        uint8_t byte = buffer[i];
        
        if (byte == ESCAPE) {
            // Escape sequence detected, XOR the next byte with 0x20
            i++;  // Move to next byte
            if (i < bufferSize) {
                processedBuffer[processedBufferSize++] = buffer[i] ^ 0x20;
            }
        } else {
            // Regular byte, add to output
            processedBuffer[processedBufferSize++] = byte;
        }
    }
}

int stuffing2(const unsigned char *buf, int bufSize, unsigned char *stuffed_buf) {
    int stuffed_bufSize = 0;

    for (int i = 0; i < bufSize; i++) {
        uint8_t byte = buf[i];
        
        if (byte == FLAG) {
            // Replace 0x7E with 0x7D 0x5E
            stuffed_buf[stuffed_bufSize++] = ESCAPE;
            stuffed_buf[stuffed_bufSize++] = byte ^ 0x20;
        } else if (byte == ESCAPE) {
            // Replace 0x7D with 0x7D 0x5D
            stuffed_buf[stuffed_bufSize++] = ESCAPE;
            stuffed_buf[stuffed_bufSize++] = byte ^ 0x20;
        } else {
            // Regular byte, add to output
            stuffed_buf[stuffed_bufSize++] = byte;
        }
    }

    return stuffed_bufSize;
}

int destuffing2(const unsigned char *buf, int bufSize, unsigned char *de_stuffed_buf) {

    int de_stuffed_bufSize = 0;
    for (int i = 0; i < bufSize; i++) {
        uint8_t byte = buf[i];
        
        if (byte == ESCAPE) {
            // Escape sequence detected, XOR the next byte with 0x20
            i++;  // Move to next byte
            if (i < bufSize) {
                de_stuffed_buf[de_stuffed_bufSize++] = buffer[i] ^ 0x20;
            }
        } else {
            // Regular byte, add to output
            de_stuffed_buf[de_stuffed_bufSize++] = byte;
        }
    }
}

uint8_t calculate_bcc(int start, int len) {
    uint8_t bcc = 0;
    for (int i = start; i < len; i++) {
        bcc ^= processedBuffer[i];  // XOR each byte
    }
    return bcc;
}

uint8_t calculate_bcc2(const unsigned char *buf, int start, int len) {
    uint8_t bcc = 0;
    for (int i = start; i < len; i++) {
        bcc ^= buf[i];  // XOR each byte
    }
    return bcc;
}

int populateFrame(Frame* frame){
    destuffing();

    frame->address = processedBuffer[0];
    frame->control = processedBuffer[1];
    frame->bcc1 = processedBuffer[2];
    if (frame->bcc1 != (frame->address^frame->control)) return 1; //BCC1 failed!

    if (processedBufferSize = 3) return 0; //Control frame
    
    frame->infoFrame.dataSize = processedBufferSize - 2;
    
    uint8_t bcc = calculate_bcc(3, frame->infoFrame.dataSize);
    if (bcc != processedBuffer[processedBufferSize - 1]) return 2; //BCC2 failed!
    
    for (int i = 0; i < frame->infoFrame.dataSize; i++) {
        frame->infoFrame.data[i] = processedBuffer[i + 3];
    }

    return 0;
}

int read_frame(Frame* frame) {
    int alarmCounter = 0; //TODO: Add alarm
    int start_found = 0;


    uint8_t byte;
    uint8_t readcount = readByte(&byte);

    if (byte == FLAG) {
    if (!start_found) {
            // Start of frame
            start_found = 1;
            bufferSize = 0;
        } else {
            // End of frame
            return populateFrame(&frame);
        }

    } else if (start_found) {

        buffer[bufferSize++] = byte;
        if (bufferSize >= sizeof(buffer)) {
            printf("Frame too large!\n");
            return -2;  // Error: frame exceeds buffer size
        }
    }  
}
    
int write_frame(Frame frame) {
    bufferSize = 0;

    bufferSize++; // Start flag
    buffer[bufferSize++] = frame.address;
    buffer[bufferSize++] = frame.control;
    buffer[bufferSize++] = frame.bcc1;

    //Add data if it's an I frame
    if (frame.infoFrame.dataSize > 0) {
        memcopy(buffer + bufferSize, frame.infoFrame.data, frame.infoFrame.dataSize);
        buffer[bufferSize++] = frame.infoFrame.bcc2;
    }

    bufferSize++; // End flag
    stuffing();

    // Add flags after stuffing
    processedBuffer[0] = FLAG;
    processedBuffer[processedBufferSize - 1] = FLAG;

    const char *bytes = processedBuffer;

    return writeBytes(bytes, processedBufferSize);;
}

int write_buf(uint8_t address, uint8_t control, const unsigned char *buf, int bufSize) {
    bufferSize = 0;

    bufferSize++; // Start flag
    buffer[bufferSize++] = address;
    buffer[bufferSize++] = control;
    buffer[bufferSize++] = address ^ control;

    if (bufSize > 0) {
        memcopy(buffer + bufferSize, buf, bufSize);
        buffer[bufferSize++] = calculate_bcc2(buf, 0, bufSize);
    }

    bufferSize++; // End flag
    char* stuffedBuf[BUFSIZE];
    int stuffedSize = stuffing2(buf, bufSize, stuffedBuf);

    // Add flags after stuffing
    processedBuffer[0] = FLAG;
    processedBuffer[stuffedSize - 1] = FLAG;

    const char *bytes = stuffedBuf;

    return writeBytes(bytes, stuffedSize);;
}

Frame create_frame(uint8_t type, uint8_t address, char* packet, int packetSize) {
    Frame frame;
    if (packetSize > sizeof(frame.infoFrame.data)){
        printf("[CRITICAL ERROR] Packet too large!\n");
        return frame;
    }
    
    frame.address = address;
    frame.control = type;
    frame.bcc1 = frame.address ^ frame.control;

    if (packetSize > 0) {
        frame.infoFrame.dataSize = packetSize;
        memcopy(frame.infoFrame.data, packet, packetSize);
        frame.infoFrame.bcc2 = calculate_bcc(0, packetSize);
    }    
    
    return frame;
}