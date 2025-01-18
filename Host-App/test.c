#include "main.h"

HANDLE hComm; 
DWORD bytesWritten, bytesReadFromPort;
#define CHUNK_SIZE 200
unsigned char buffer[CHUNK_SIZE];
FILE *file;
size_t bytesRead;
unsigned short crc;
uint8_t ackBuffer[2];
int retryCount;
uint32_t data;




/* Table of CRC values for high-order byte */
static const unsigned char table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const unsigned char table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

void SerialConfiguration(const char * ComPort,const char * FilePath)
{
   BOOL Status;
   hComm = CreateFile(
        ComPort,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
   );
   if(hComm == INVALID_HANDLE_VALUE )
   {
        printf("\n   Error! - Port %s can't be opened", ComPort);
        printf("\n   Check board connection and Port Number\n");
        exit(-1);
   }
   else
    {
        printf("\n Port %s Opened \n", ComPort);

    }

    /****************************************Setting the Parameters of for the SerialPort ********************************/
    DCB dcb ={0};
    dcb.DCBlength = sizeof(dcb);
    Status = GetCommState(hComm, &dcb);//retreives the current settings

    if(Status == FALSE)
         printf("**************Error in CommState()**************\r\n");

    dcb.BaudRate = 115200;
    dcb.ByteSize = 8; 
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY; 

    Status = SetCommState(hComm,&dcb);

    if(Status == FALSE )
    {
       printf("Error ! in Setting DCB Structure");
    }
    else{
        printf("\n   Setting DCB Structure Successfull\n");
        printf("\n       Baudrate = %ld", dcb.BaudRate);
        printf("\n       ByteSize = %d", dcb.ByteSize);
        printf("\n       StopBits = %d", dcb.StopBits);
        printf("\n       Parity   = %d", dcb.Parity);
    }

    /*------------------------------------ Setting Timeouts --------------------------------------------------*/
#if 1
    COMMTIMEOUTS timeouts = { 0 };

    timeouts.ReadIntervalTimeout         = 300;
    timeouts.ReadTotalTimeoutConstant    = 300;
    timeouts.ReadTotalTimeoutMultiplier  = 300;
    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (SetCommTimeouts(hComm, &timeouts) == FALSE)
        printf("\n   Error! in Setting Time Outs");
    else
        printf("\n\n   Setting Serial Port Timeouts Successfull");

    Status = SetCommMask(hComm, EV_RXCHAR); //Configure Windows to Monitor the serial device for Character Reception

    if (Status == FALSE)
        printf("\n\n   Error! in Setting CommMask");
    else
        printf("\n\n   Setting CommMask successfull");
#endif
}

uint32_t read_Serial_port(uint8_t * pBuffer, uint32_t Len )
{
    uint32_t no_of_bytes_read;  
    ReadFile(hComm , pBuffer, Len , &no_of_bytes_read, NULL);

    return no_of_bytes_read;
}

//closes the serial port
void Close_serial_port(void)
{
    CloseHandle(hComm);//Closing the Serial Port
}
//explore : https://msdn.microsoft.com/en-us/library/windows/desktop/aa363428(v=vs.85).aspx
void purge_serial_port(void)
{
     PurgeComm(hComm,PURGE_RXCLEAR|PURGE_TXCLEAR);

}


//This fun is used to Send data over the Serial port of "len" bytes
void Write_Serial_Port(uint8_t * data_buf, uint32_t len)
{
    DWORD dNoOfBytesWritten = 0; 
    BOOL Status; 
    Status = WriteFile(hComm, data_buf, len, &dNoOfBytesWritten, NULL);
    if(Status == TRUE)
    {
        printf("\r\n Sending Command :\n");
        for(uint32_t i = 0 ; i < len ; i++)
        {
            printf("   0x%2.2x ",data_buf[i]);
            if( i % 8 == 7)
            {
                printf("\n");
            }
        }
    }
    else
        printf("\n  Error %ld in Writing to Serial Port",GetLastError());
    
}

void SendDataforPrint(char *Buffer, uint16_t Len)
{
  printf("Sending Chunks : Bytes[%d] \t",Len);
   for(int i =0 ; i< Len; i++)
   {
    printf("0x%2.2x ",Buffer[i]);
   }
   printf("\r\n");
   printf("\n\n");
}

unsigned int crc16(unsigned char *buffer, unsigned int buffer_length)
{
    unsigned char crc_hi = 0xFF; /* high CRC byte initialized */
    unsigned char crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_lo ^ *buffer++; /* calculate the CRC  */
        crc_lo = crc_hi ^ table_crc_hi[i];
        crc_hi = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}
 
void SendingFirmware_File(const char * FILEPATH)
{
        FILE *file;
    unsigned char buffer[CHUNK_SIZE]; // Chunk size
    unsigned int crc_value;
    uint8_t crc_load[] = {0x00, 0x00}; // High, Low 
    size_t read_size;
    BOOL Error = 0;

    file = fopen(FILEPATH, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Read and send in chunks
    while ((read_size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // SendDataforPrint(buffer, read_size);
        printf("%d",read_size);

        crc_load[0] = read_size;
        Write_Serial_Port(crc_load , 1);
        Write_Serial_Port(buffer, read_size);  
        crc_value = crc16(buffer , read_size);
        crc_load[0] = (crc_value >>0x08) & 0xFF;
        crc_load[1] = (crc_value ) & 0xFF;
        // SendDataforPrint(crc_load,2);
        Write_Serial_Port(crc_load, 2);
        ackBuffer[0] = ackBuffer[1]=0;
        data =0;
        while(!(data = read_Serial_port(ackBuffer,sizeof(ackBuffer)/sizeof(uint8_t))));
        SendDataforPrint(ackBuffer,2);
        if((ackBuffer[0] == 0xFF) && (ackBuffer[1] == 0xFF))
        {
            printf("\r\n :: Successfully Transmitted :: \r\n");
            Error |= 0;
        }
        else{
            printf("\r\n :: Terminated :: \r\n");
            Error |= 1;
            break;
        }
        // if(ackBuffer)
        
    }
    if(Error == 0)
    {
        uint8_t End_Frame_Buffer[3];
        End_Frame_Buffer[0] = 'E';
        End_Frame_Buffer[1] = 'O';
        End_Frame_Buffer[2] = 'F';
        crc_load[0] = 3;
        Write_Serial_Port(crc_load , 1);
        Write_Serial_Port(End_Frame_Buffer, 3); 
        crc_value = crc16(End_Frame_Buffer , 3);
        crc_load[0] = (crc_value >>0x08) & 0xFF;
        crc_load[1] = (crc_value ) & 0xFF;
        // SendDataforPrint(crc_load,2);
        Write_Serial_Port(crc_load, 2);

        ackBuffer[0] = ackBuffer[1]=0;
        data =0;
        while(!(data = read_Serial_port(ackBuffer,sizeof(ackBuffer)/sizeof(uint8_t))));
        SendDataforPrint(ackBuffer,2);
        if((ackBuffer[0] == 0xEF) && (ackBuffer[1] == 0xEF))
        {
            printf("\r\n :: EOF Successfully Transmitted :: \r\n");
            Error |= 0;
        }
        else{
            printf("\r\n :: Terminated :: \r\n");
            Error |= 1;
        }

    }

    fclose(file);
    return 0;
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <COM_PORT> <FILE_PATH>\n", argv[0]);
        printf("Example: %s COM3 user_app.bin\n", argv[0]);
        return 1;
    }

    const char *comPort = argv[1];
    const char *filePath = argv[2];

    SerialConfiguration(comPort,filePath);
  #if 0
   uint8_t Buffer[] = {0x01,0x03};
   Write_Serial_Port(Buffer, sizeof(Buffer)/sizeof(uint8_t));   


   uint8_t Buffer1 [] = {0x01,0x03,0x01,0x03,0x01,0x03};
    Write_Serial_Port(Buffer1, sizeof(Buffer1)/sizeof(uint8_t));

   Write_Serial_Port("HELLOWORLD", sizeof("HELLOWORLD")/sizeof(uint8_t));
   #endif

 while(1)
 {
   while(!(data = read_Serial_port(ackBuffer,sizeof(ackBuffer)/sizeof(uint8_t))));
   uint8_t Buffer[] = {0x01,0x03};
   printf("%d",data);
   printf("ackBuffer : %d \t %d",ackBuffer[0], ackBuffer[1]);
   switch(ackBuffer[0])
   {
    case 0x01: 
        //Start Bit Send  SOF 
       if(ackBuffer[1] == 0x03)
           Write_Serial_Port(Buffer, sizeof(Buffer)/sizeof(uint8_t));   
           SendingFirmware_File(filePath);
           return 0;
    break;
    default: 
    break; 
   }
 }
    
  return 0;
}