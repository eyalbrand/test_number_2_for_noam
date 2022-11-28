// Includes ------------------------------------------------------------------------------
#include "log.h"

/// <summary>
/// create a handler that enables writing into destination file
/// </summary>
/// <param name="file_name">file name</param>
/// <returns>handler</returns>
HANDLE file_write_handler(char* file_name) {
    HANDLE h_new_file_to_create = CreateFileA
    (file_name,							      // file to open
        GENERIC_WRITE,		                  // open writing
        FILE_SHARE_WHILE_READ_OR_WRITE,       // not share
        NULL,
        OPEN_EXISTING,						  // existing file only
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (h_new_file_to_create == INVALID_HANDLE_VALUE) {
        printf("Terminal failure: Unable to open file %s for write\n", file_name);
        printf("last error was %d", GetLastError());
        CloseHandle(h_new_file_to_create);
        exit(EXIT_FAILURE);
    }
    else
        return h_new_file_to_create;
}


/// <summary>
/// the function the writes messages to log. differs between sent/recive messages and error messages
/// </summary>
/// <param name="file_name">the file name</param>
/// <param name="send_or_recived">send or recive message</param>
/// <param name="server_or_client">server or client</param>
/// <param name="raw_message">the message to sent</param>
/// <param name="type_message">sent/recive or error</param>
void write_to_log_file(char* file_name, char* send_or_recived, char* server_or_client, char* raw_message, int type_message) {
    int log_file_offset = 0;
    char message_buffer[MAX_MESSAGE_TO_LOG] = { 0 };
    HANDLE file_log_handle = file_write_handler(file_name);
    DWORD offset_to_write_file = SetFilePointer(file_log_handle, 0, NULL, FILE_END);

    if (type_message == MESSAGE_REGULER) {
        sprintf(message_buffer, "%s from %s-%s", send_or_recived, server_or_client, raw_message);
        if (FALSE == WriteFile(file_log_handle, message_buffer, strlen(message_buffer), NULL, NULL)) { //write the message to log file
            printf("Terminal failure: Unable to write to %s.txt\n", file_name);
            CloseHandle(file_log_handle);
            exit(EXIT_FAILURE); // need to decide
        }
        CloseHandle(file_log_handle);
    }

    else {
        if (FALSE == WriteFile(file_log_handle, raw_message, strlen(raw_message), NULL, NULL)) { //write the message to log file
            printf("Terminal failure: Unable to write to %s.txt\n", file_name);
            CloseHandle(file_log_handle);
            exit(EXIT_FAILURE); // need to decide
        }
        CloseHandle(file_log_handle);
    }
}