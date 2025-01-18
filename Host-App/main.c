#if 0
#include <windows.h>
#include <stdio.h>
#include <stdlib.h> // For exit()

#define CHUNK_SIZE 256  // Size of each chunk to send

void sendFileOverUART(const char *comPort, const char *filePath) {
    HANDLE hSerial;
    DWORD bytesWritten;
    char buffer[CHUNK_SIZE];
    FILE *file;
    size_t bytesRead;

    // Open the serial port
    hSerial = CreateFile(comPort, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening serial port %s.\n", comPort);
        return;
    }

    // Configure serial port
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printf("Error getting serial port state.\n");
        CloseHandle(hSerial);
        return;
    }
    dcbSerialParams.BaudRate = CBR_115200;  // Set baud rate (e.g., 115200)
    dcbSerialParams.ByteSize = 8;           // Data bits
    dcbSerialParams.StopBits = ONESTOPBIT;  // Stop bit
    dcbSerialParams.Parity = NOPARITY;      // Parity
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printf("Error setting serial port state.\n");
        CloseHandle(hSerial);
        return;
    }

    // Open the file to be sent
    file = fopen(filePath, "rb");
    if (!file) {
        printf("Error opening file: %s\n", filePath);
        CloseHandle(hSerial);
        return;
    }

    printf("Sending file: %s to COM port: %s\n", filePath, comPort);

    // Send the file in chunks
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (!WriteFile(hSerial, buffer, (DWORD)bytesRead, &bytesWritten, NULL)) {
            printf("Error writing to serial port.\n");
            break;
        }
        printf("Sent chunk: %lu bytes\n", bytesWritten);
    }

    printf("File transmission complete.\n");

    // Close file and serial port
    fclose(file);
    CloseHandle(hSerial);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <COM_PORT> <FILE_PATH>\n", argv[0]);
        printf("Example: %s COM3 user_app.bin\n", argv[0]);
        return 1;
    }

    const char *comPort = argv[1];  // Get COM port from argument
    const char *filePath = argv[2]; // Get file path from argument

    sendFileOverUART(comPort, filePath);
    return 0;
}

#endif 

#include <windows.h>
#include <stdio.h>
#include <stdlib.h> // For exit()

#define CHUNK_SIZE 200  // Size of each chunk to send
#define RETRY_LIMIT 3   // Number of retries for failed transmission
#define END_MARKER "EOF"

// Function to calculate CRC16
unsigned short calculateCRC(const unsigned char *data, size_t length) {
    unsigned short crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (unsigned char j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// Send file over UART
void sendFileOverUART(const char *comPort, const char *filePath) {
    HANDLE hSerial;
    DWORD bytesWritten, bytesReadFromPort;
    char buffer[CHUNK_SIZE];
    FILE *file;
    size_t bytesRead;
    unsigned short crc;
    char ackBuffer[2];
    int retryCount;

    // Open the serial port
    hSerial = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening serial port %s.\n", comPort);
        return;
    }

    // Configure serial port
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printf("Error getting serial port state.\n");
        CloseHandle(hSerial);
        return;
    }
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printf("Error setting serial port state.\n");
        CloseHandle(hSerial);
        return;
    }

    // Open the file to be sent
    file = fopen(filePath, "rb");
    if (!file) {
        printf("Error opening file: %s\n", filePath);
        CloseHandle(hSerial);
        return;
    }

    printf("Sending file: %s to COM port: %s\n", filePath, comPort);

    // Define the start marker
    unsigned char startMarker[3] = {0x01, 0x03};
    if (!WriteFile(hSerial, startMarker, sizeof(startMarker), &bytesWritten, NULL)) {
        printf("Error sending start marker.\n");
        fclose(file);
        CloseHandle(hSerial);
        return;
    }

    printf("Start marker sent.\n");

    // Send the file in chunks
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        // Calculate CRC for the chunk
        crc = calculateCRC((unsigned char *)buffer, bytesRead);

        // Retry logic for sending a chunk
        retryCount = 0;
        while (retryCount < RETRY_LIMIT) {
            // Send chunk
            if (!WriteFile(hSerial, buffer, (DWORD)bytesRead, &bytesWritten, NULL)) {
                printf("Error writing chunk to serial port.\n");
                retryCount++;
                continue;
            }

            // Send CRC
            if (!WriteFile(hSerial, &crc, sizeof(crc), &bytesWritten, NULL)) {
                printf("Error writing CRC to serial port.\n");
                retryCount++;
                continue;
            }

            // Wait for acknowledgment
            if (ReadFile(hSerial, ackBuffer, 1, &bytesReadFromPort, NULL) && ackBuffer[0] == 0x06) {
                printf("Chunk sent and acknowledged.\n");
                break; // Successful transmission
            } else {
                printf("No acknowledgment received. Retrying...\n");
                retryCount++;
            }
        }

        if (retryCount == RETRY_LIMIT) {
            printf("Failed to send chunk after %d retries. Aborting.\n", RETRY_LIMIT);
            fclose(file);
            CloseHandle(hSerial);
            return;
        }
    }

    // Send end marker
    if (!WriteFile(hSerial, END_MARKER, strlen(END_MARKER), &bytesWritten, NULL)) {
        printf("Error sending end marker.\n");
    } else {
        printf("End marker sent.\n");
    }

    printf("File transmission complete.\n");

    // Close file and serial port
    fclose(file);
    CloseHandle(hSerial);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <COM_PORT> <FILE_PATH>\n", argv[0]);
        printf("Example: %s COM3 user_app.bin\n", argv[0]);
        return 1;
    }

    const char *comPort = argv[1];
    const char *filePath = argv[2];

    sendFileOverUART(comPort, filePath);
    return 0;
}
