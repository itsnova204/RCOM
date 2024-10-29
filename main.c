#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "application_layer.h"
#include "frame.h" // Ensure frame and state machine logic is included here

#define N_TRIES 3
#define TIMEOUT 4

void applicationLayer2(const char *serialPort, const char *role, int baudrate, int tries, int timeout, const char *filename) {
    Frame frame;

    if (strcmp(role, "tx") == 0) {
        // Simulate sending frames (tx mode)
        printf("Transmitter Mode - Sending Frames\n");

        uint8_t validFrame[] = {
            0x7E, 0x03, 0x03, 0x00, 0x05, 'H', 'E', 'L', 'L', 'O', 0x25, 0x7E
        };

        uint8_t invalidBCC2Frame[] = {
            0x7E, 0x03, 0x03, 0x00, 0x05, 'W', 'O', 'R', 'L', 'D', 0x99, 0x7E
        };

        uint8_t shortFrame[] = {
            0x7E, 0x03, 0x03, 0x00, 0x00, 0x7E
        };

        // Simulate writing the frames
        printf("Sending a valid frame...\n");
        for (int i = 0; i < sizeof(validFrame); i++) {
            stateMachine(validFrame[i], &frame);
        }
        if (validateFrame(frame) == 0) {
            printf("Valid frame sent successfully!\n");
        }

        printf("Sending a frame with invalid BCC2...\n");
        for (int i = 0; i < sizeof(invalidBCC2Frame); i++) {
            stateMachine(invalidBCC2Frame[i], &frame);
        }
        if (validateFrame(frame) == 2) {
            printf("Invalid BCC2 detected in sent frame.\n");
        }

        printf("Sending a short control frame...\n");
        for (int i = 0; i < sizeof(shortFrame); i++) {
            stateMachine(shortFrame[i], &frame);
        }
        if (validateFrame(frame) == 0) {
            printf("Short control frame sent successfully!\n");
        }

    } else if (strcmp(role, "rx") == 0) {
        // Simulate receiving frames (rx mode)
        printf("Receiver Mode - Waiting for Frames\n");

        // Simulate receiving and validating the same frames
        uint8_t receivedFrame[] = {
            0x7E, 0x03, 0x03, 0x00, 0x05, 'H', 'E', 'L', 'L', 'O', 0x25, 0x7E
        };

        printf("Receiving a valid frame...\n");
        for (int i = 0; i < sizeof(receivedFrame); i++) {
            stateMachine(receivedFrame[i], &frame);
        }

        int validationResult = validateFrame(frame);
        if (validationResult == 0) {
            printf("Valid frame received successfully!\n");
        } else {
            printf("Error in received frame. Error code: %d\n", validationResult);
        }
    }
}

// Main function provided earlier remains unchanged
int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s /dev/ttySxx baudrate tx|rx filename\n", argv[0]);
        exit(1);
    }

    const char *serialPort = argv[1];
    const int baudrate = atoi(argv[2]);
    const char *role = argv[3];
    const char *filename = argv[4];

    // Validate baud rate
    switch (baudrate) {
        case 1200:
        case 1800:
        case 2400:
        case 4800:
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
            break;
        default:
            printf("Unsupported baud rate (must be one of 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200)\n");
            exit(2);
    }

    // Validate role
    if (strcmp("tx", role) != 0 && strcmp("rx", role) != 0) {
        printf("ERROR: Role must be \"tx\" or \"rx\"\n");
        exit(3);
    }

    printf("Starting link-layer protocol application\n"
           "  - Serial port: %s\n"
           "  - Role: %s\n"
           "  - Baudrate: %d\n"
           "  - Number of tries: %d\n"
           "  - Timeout: %d\n"
           "  - Filename: %s\n",
           serialPort,
           role,
           baudrate,
           N_TRIES,
           TIMEOUT,
           filename);

    applicationLayer2(serialPort, role, baudrate, N_TRIES, TIMEOUT, filename);

    return 0;
}
