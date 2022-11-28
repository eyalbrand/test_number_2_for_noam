#pragma once
#ifndef __service_thread_h__
#define __service_thread_h__

#include <stdio.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include<stdbool.h>
#include <winsock2.h>
#include <time.h>

#define SEND "sent"
#define RECIVED "received"
#define ERROR_MESSAGE "error"
#define CLIENT "client"
#define SERVER "server"
#define ERROR_MESSAGE "error"
#define MESSAGE_ERROR_TYPE 555
#define MESSAGE_REGULER 111
#define MAX_MESSAGE_TO_LOG 100
#define MAX_TIMEOUT = 30
DWORD service_thread_func(SOCKET* t_socket);
TransferResult_t SendToClient(const char* SendStr, SOCKET t_socket);

TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);
TransferResult_t SendString(const char* Str, SOCKET sd);
TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd);
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd);

int movesOfPlayers[NUM_OF_WORKER_THREADS];
char* usernames[NUM_OF_WORKER_THREADS];



static HANDLE _mutex_REQUEST = NULL;
static HANDLE _mutex_GAME_S = NULL;
static HANDLE _mutex_BEGIN = NULL;

static LPCTSTR P_REQUEST_MUTEX;
static LPCTSTR P_GAME_S_MUTEX;
static LPCTSTR P_BEGIN_MUTEX;

#endif