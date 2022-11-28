#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//define decleration-----------------------------------------------
#define ADDRESS_LEN 22
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 ) 
#define MAX_ORDER 30
#define MAX_USER_MOVE 30
#define MAX_USER_NAME 20
#define USER_WANT_OUT -1
#define MESSAGE_ERROR_TYPE 555
#define MESSAGE_REGULER 111
#define ERROR_SERVER_DISCONNECT 10054
#define MAX_MESSAGE_TO_LOG 100


// Var decleration---------------------------------------------
SOCKET m_socket;
SOCKADDR_IN clientService;
typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;
int SERVER_PORT;
char SERVER_PORT_STR[ADDRESS_LEN] = "";
char SERVER_ADDRESS_STR[ADDRESS_LEN] = "";
char user_name[MAX_USER_NAME] = "";
char log_file_name[MAX_USER_NAME + ADDRESS_LEN] = { 0 };
char sent[] = "sent";
char recivied[] = "received";
char error[] = "error";
char client[] = "client";
char server[] = "server";
int g_user_choosing = -1;
int g_server_disconnection = 0;


// function decleration--------------------------------------------------------
int create_connection();
int RecvData();
int call_the_order(char order[], char p[]);
HANDLE create_file(char* file_name);
TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);
TransferResult_t SendString(const char* Str, SOCKET sd);
TransferResult_t ReceiveBuffer(char* OutputBuffer, int RemainingBytesToReceive, SOCKET sd);
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd);

