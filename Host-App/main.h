/*
Author : Harsh Dixit 
Date : 17/01/2025

*/
#ifndef INC_MAIN_H_
#define INC_MAIN_H_


#include <Windows.h>
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#define WINDOWS_HOST

void SerialConfiguration(const char * ComPort,const char * FilePath);

unsigned int crc16(unsigned char *buffer, unsigned int buffer_length);

#endif