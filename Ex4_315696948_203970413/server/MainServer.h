#pragma once
#ifndef __server_header_h__
#define __server_header_h__
#include "HardCoded.h"

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
DWORD service_thread_func(SOCKET* t_socket);
typedef enum { TRNS_FAILED, TRNS_DISCONNECTED, TRNS_SUCCEEDED } TransferResult_t;

typedef struct userObj {
	char  name[MAX_NAME_LEN];
	int won;
	int lost;
	double ratio;
} userObj;
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

DWORD WINAPI inputThread(LPVOID param);
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID parameter, LPDWORD p_thread_id);

#endif