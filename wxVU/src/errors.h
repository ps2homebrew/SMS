#ifndef _ERRORS 
#define _ERRORS
#include "datatypes.h"

const int32     E_TIMEOUT       =   -10;
// VU Errors
 

// PS2Link protocol errorrs 
const int32     E_SOCKET        =   -20;
const int32     E_NO_LINK       =   -21;
const int32     E_LINK_TIMEOUT  =   -22;
const int32     E_SOCK_CLOSE    =   -23;

// File operations errors
const int32     E_FILE_OPEN     =   -30;
const int32     E_FILE_READ     =   -31;
const int32     E_FILE_WRITE    =   -32;
#endif
