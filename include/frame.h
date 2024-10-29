#pragma once

#include <stdint.h>

#define FLAG 0x7E
#define MAX_DATA_SIZE 256

#define SET 0x03
#define UA 0x07
#define RR0 0xAA
#define RR1 0xAB
#define REJ0 0x54
#define REJ1 0x55
#define DISC 0x0B

// States for the state machine
typedef enum {
    STATE_IDLE,
    STATE_ADDRESS,
    STATE_CONTROL,
    STATE_FRAME_TYPE,
    STATE_LENGTH,
    STATE_DATA,
    STATE_BCC1,
    STATE_BCC2,
    STATE_END
} State;

typedef struct {
    uint8_t type;    //type 0 - filesize, 1 - filename
    uint8_t size;       // Size of the data field
    uint8_t data[MAX_DATA_SIZE]; // Data field of the I frame
} ControlPacket;

typedef struct {
    uint8_t sequenceNumber;  // Sequence number of the I frame (0-99)
    uint16_t dataSize;       // Size of the data field
    uint8_t data[MAX_DATA_SIZE]; // Data field of the I frame
} InformationPacket;

// Structure Definitions
typedef struct {
    uint8_t type;    // Control frame type: SET, UA, RR, REJ, DISC
} ControlFrame;

typedef struct {
    uint8_t dataSize;       // Size of the data field
    uint8_t data[MAX_DATA_SIZE]; // Data field of the I frame
    uint8_t bcc2; 
} InformationFrame;

typedef struct {
    uint8_t address;  // Address Field (common for both frames)
    uint8_t control;  // Control Field (common for both frames)
    uint8_t bcc1;     // BCC1 for header error checking
    InformationFrame infoFrame; // Information Frame (if it's an I frame)
} Frame;




