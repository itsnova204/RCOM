// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

typedef struct {
    int value;      // integer value
    unsigned char *pointer;  // pointer to a character array
} Result;

// Creates Control packet
// returns pointer to the packet and its size
Result getControlPacket(int control_field, const char *file_name, long file_size);

// Creates Data packet
// returns pointer to the packet and its size
Result getDataPacket(unsigned char *buf, int buf_size, int sequence_number);

// Parses I and C packets
// saves data from data packets into the file
int parsePacket(unsigned char *packet, int size, FILE* fptr);

#endif // _APPLICATION_LAYER_H_
