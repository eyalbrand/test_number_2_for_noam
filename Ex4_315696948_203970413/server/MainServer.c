
#include "MainServer.h"
#include "global.h"

#include<winsock2.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include<stdbool.h>
#define WIN32_LEAN_AND_MEAN

HANDLE ThreadHandles[NUM_OF_WORKER_THREADS+1];
SOCKET ThreadInputs[NUM_OF_WORKER_THREADS+1];

/// <summary>
/// poll the console every 2 seconds
/// </summary>
/// <param name="socket"> socket</param>
/// <param name="sec"> time to poll </param>
/// <param name="usec"> micro second </param>
/// <returns></returns>
int recvTimeOutTCP(SOCKET socket, long sec, long usec)

{
	// Setup timeval variable
	struct timeval timeout;
	struct fd_set fds;

	// assign the second and microsecond variables
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;
	// Setup fd_set structure
	FD_ZERO(&fds);
	FD_SET(socket, &fds);
	// Possible return values:
	// -1: error occurred
	// 0: timed out
	// > 0: data ready to be read
	return select(0, &fds, 0, 0, &timeout);
}


/// <summary>
/// in the end of the program. this functuons checks that every client/thread is done. it does so in loop over the ThreadHandles.
/// </summary>
static void CleanupWorkerThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				if (!ClosingAlert){
					printf("Waiting for thread failed. Ending program\n");
				}
				return;
			}
		}
	}
}

static void CleanupBadThread(int index)
{

	if (ThreadHandles[index] != NULL)
	{
		// poll to check if thread finished running:
		DWORD Res = WaitForSingleObject(ThreadHandles[index], 0);

		if (Res == WAIT_OBJECT_0)
		{
			closesocket(ThreadInputs[index]);
			CloseHandle(ThreadHandles[index]);
			ThreadHandles[index] = NULL;
			printf("Error with thread (thread number: %d)\n", index);
		}
		else
		{
			if (!ClosingAlert) {
				printf("Waiting for thread failed. Ending program\n");
			}
			return;
		}
	}
	
}

/// <summary>
/// clean all the worker threads in the server when exiting.
/// </summary>
static void CleanupAllThreads()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS + 1; Ind++)
	{
		if (ThreadHandles[Ind] != NULL)
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0)
			{
				closesocket(ThreadInputs[Ind]);
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
			}
			else
			{
				if (!ClosingAlert) {
					printf("Waiting for thread failed. Ending program\n");
				}
				return;
			}
		}
	}
}


/// <summary>
/// open the socket, listen on it and create the threads for the clients.
/// </summary>
/// <param name="port">an integer which contains the port number.</param>
/// <returns>if success, return handle to the new thread handle, if not it return (-1)</returns>
int MainServer(int port) {
	int Ind;
	SOCKET MainSocket = INVALID_SOCKET;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	DWORD wait_code;
	int SelectTiming;
	BOOL ret_val;
	size_t i;
	DWORD p_thread_ids[NUM_OF_WORKER_THREADS + 1];


	// Initialize Winsock - Start communication. 
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (StartupRes != NO_ERROR)
	{
		printf("error %ld at WSAStartup( ), ending program.\n", WSAGetLastError());
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return ERROR_CODE;
	}

	/* The WinSock DLL is acceptable. Proceed. */

	// Create a socket.    
	MainSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (MainSocket == INVALID_SOCKET)
	{
		printf("Error at socket( ): %ld\n", WSAGetLastError());
		goto server_cleanup_1;
	}
	// Bind the socket.
	/*
		For a server to accept client connections, it must be bound to a network address within the system.
		The following code demonstrates how to bind a socket that has already been created to an IP address
		and port.
		Client applications use the IP address and port to connect to the host network.
		The sockaddr structure holds information regarding the address family, IP address, and port number.
		sockaddr_in is a subset of sockaddr and is used for IP version 4 applications.
   */
   // Create a sockaddr_in object and set its values.
   // Declare variables

	Address = inet_addr("127.0.0.1");
	if (Address == INADDR_NONE)
	{
		goto server_cleanup_2;
	}

	service.sin_family = AF_INET;
	service.sin_addr.s_addr = Address;
	service.sin_port = htons(port); //The htons function converts a u_short from host to TCP/IP network byte order 
									   //( which is big-endian ).
	/*
		The three lines following the declaration of sockaddr_in service are used to set up
		the sockaddr structure:
		AF_INET is the Internet address family.
		Address is the local IP address to which the socket will be bound.
		the port number to which the socket will be bound we get from command line argument.
	*/

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	bindRes = bind(MainSocket, (SOCKADDR*)&service, sizeof(service));
	if (bindRes == SOCKET_ERROR)
	{
		printf("bind( ) failed with error %ld. Ending program\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Listen on the Socket.

	ListenRes = listen(MainSocket, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		goto server_cleanup_2;
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++)
		ThreadHandles[Ind] = NULL;

	printf("Waiting for a client to connect...\n");

	DWORD lpExitCode; //handle exit code
	while (TRUE) // wait 15 seconds
	{

		if (ClosingAlert){
			printf("received exit from console, exiting\n"); // for debug.
			CleanupAllThreads();
			p_thread_ids[0] = NULL; // nullify the threads, to prepare for another round.
			p_thread_ids[1] = NULL;
			p_thread_ids[2] = NULL;
			return 0;
		}

		SelectTiming = recvTimeOutTCP(MainSocket, 2, 10); // poll the console every 2 seconds
		switch (SelectTiming)
		{
		case 0: // Timed out
//			printf("Timeout waiting on accept.\n"); // for debug
			
			for (int i = 0; i < Ind + 1; i++) {
				GetExitCodeThread(ThreadHandles[i], &lpExitCode);
				if (lpExitCode == ERROR_CODE) { //print to log?

					CleanupBadThread(i);
					//return ERROR_CODE;
				}
			}
			break;
		default: {
			SOCKET AcceptSocket = accept(MainSocket, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET)
			{
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
				goto server_cleanup_3;
			}

			printf("A new client is connecting the server..\n");
			Ind = FindFirstUnusedThreadSlot();
			if (Ind != 2) { // then a third client is trying to connect
				g_first_turn = !Ind; // determine the first turn.
				g_current_turn = !Ind;
			}
			if (Ind == 0) { // to set the first player.
				g_first_turn = Ind; // determine the first turn.
				g_current_turn = Ind;
			}

			ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)service_thread_func,
				&(ThreadInputs[Ind]),
				0,
				&p_thread_ids[Ind]
			);
			}

		}

	}

	wait_code = WaitForMultipleObjects(// Wait for IO thread to receive exit command and terminate
		NUM_OF_WORKER_THREADS,				 // number of objects in array
		ThreadHandles,		 // array of objects
		TRUE,					 // wait for any object
		INFINITE);                  // Waiting forever for the theread to end - the worst case

	if (WAIT_TIMEOUT == wait_code)
	{
		printf("Timeout error when waiting\n");
		return ERROR_CODE;
	}
	else if (WAIT_FAILED == wait_code)
	{
		if (ClosingAlert != 1)
			printf("WaitForMultipleObjects has failed\n");
		return ERROR_CODE;
	}

	else if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting\n");
		return ERROR_CODE;
	}

	DWORD lpExitCodes; //handle exit code

	for (i = 0; i < NUM_OF_WORKER_THREADS; i++) {
		GetExitCodeThread(ThreadHandles[i], &lpExitCodes);
		if (lpExitCodes == ERROR_CODE) {
			printf("Error with thread (thread number: %d)\n", i);
			return ERROR_CODE;
		}
	}

	for (i = 0; i < NUM_OF_WORKER_THREADS; i++) // Close thread handles
	{
		ret_val = CloseHandle(ThreadHandles[i]);
		if (false == ret_val)
		{
			printf("Error when closing thread number %d \n", i);
			return ERROR_CODE;
		}
	}

server_cleanup_3:

	CleanupWorkerThreads();

server_cleanup_2:
	if (closesocket(MainSocket) == SOCKET_ERROR) {
		printf("Failed to close MainSocket, error %ld. Ending program\n", WSAGetLastError());
		return ERROR_CODE;
	}
server_cleanup_1:
	if (WSACleanup() == SOCKET_ERROR) {
		printf("Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		return ERROR_CODE;
	}

	return SUCCES_CODE;
}


/// <summary>
/// in a loop, it checks if there is a vacant spot in the ThreadHandles array to create a new room for a new client / thread.  if there is no room, it checks if one clients / threads done and take its place.
/// </summary>
/// <returns>an int, which is the vacan index</returns>
static int FindFirstUnusedThreadSlot()
{
	int Ind;

	for (Ind = 0; Ind < NUM_OF_WORKER_THREADS + 1; Ind++)
	{
		if (ThreadHandles[Ind] == NULL)
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject(ThreadHandles[Ind], 0);

			if (Res == WAIT_OBJECT_0) // this thread finished running
			{
				CloseHandle(ThreadHandles[Ind]);
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}


/// <summary>
/// A simplified API for creating threads, just a wrapper for CreateThread.
/// </summary>
/// <param name="p_start_routine">A pointer to the function to be executed by the thread. This pointer represents the starting address of the thread.</param>
/// <param name="parameter">DWORD WINAPI FunctionName(LPVOID lpParam). The function name differs from thread to thread.</param>
/// <param name="p_thread_id">A pointer to a variable that receives the thread identifier.</param>
/// <returns></returns>
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID parameter, LPDWORD p_thread_id)
{
	HANDLE thread_handle;

	if (NULL == p_start_routine)
	{
		printf("Error when creating a thread\n");
		printf("Received null pointer\n");
		exit(ERROR_CODE);
	}

	if (NULL == parameter)
	{
		printf("Error when creating a thread\n");
		printf("Received null pointer\n");
		exit(ERROR_CODE);
	}

	if (NULL == p_thread_id)
	{
		printf("Error when creating a thread\n");
		printf("Received null pointer\n");
		exit(ERROR_CODE);
	}

	thread_handle = CreateThread(
		NULL,            /*  default security attributes */
		0,               /*  use default stack size */
		p_start_routine, /*  thread function */
		parameter,      /*  argument to thread function */
		0,               /*  use default creation flags */
		p_thread_id);    /*  returns the thread identifier */

	if (NULL == thread_handle)
	{
		printf("Couldn't create thread\n");
		exit(ERROR_CODE);
	}

	return thread_handle;
}


/// <summary>
/// this is the function of the thread that check if we get exit command from the server.
/// </summary>
/// <param name="param"> param to thread if needed</param>
/// <returns></returns>
DWORD WINAPI inputThread(LPVOID param)
{
	char input[MAX_LEN_OF_LINE];
	int scanfres = 0;
	while (TRUE)
	{
		scanfres = scanf("%s", input);
		if (STRINGS_ARE_EQUAL(input, "exit")) break;
	}
	//kill program, kill threads
	ClosingAlert = 1;
}