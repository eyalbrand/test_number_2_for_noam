#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS_GLOBALS
#define FILE_SHARE_WHILE_READ_OR_WRITE 0
#define MESSAGE_ERROR_TYPE 555
#define MESSAGE_REGULER 111
#define MAX_MESSAGE_TO_LOG 100

// include decleration ------------------------------------------------------
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// Function decleration -------------------------------------------------------------------
HANDLE file_write_handler(char* file_name);
void write_to_log_file(char* file_name, char* send_or_recived, char* server_or_client, char* raw_message, int type_message);
