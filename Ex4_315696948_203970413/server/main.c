/*
Authors - Eyal Brand 203970413
		  Ori Noy 315696948

Description - The project has two parts: the server, which get a port string and manage all the comunications
between maximum 2 clients playing the game "7 boom".
The client gets ip address, port and the client name for proper functioning.
*/
/// include ------------------------------------------------


#include "main.h"
#include "MainServer.h"
#include "global.h"


/// <summary>
/// this function calls to MainSever function, and creates thread that all his purpose is to check if we get the command 'exit' in the server. if so, the program terminates.
/// </summary>
/// <param name="argc">CMD string</param>
/// <param name="argv">CMD string</param>
/// <returns>if all went well it return 0, if not it return (-1).</returns>
int main(int argc, char* argv[]) {

	HANDLE p_thread_handle;
	DWORD p_thread_id;
	DWORD wait_code;
	BOOL ret_val;
	int param = 1;
	int retFromServer;
	int port;

	
	if (argc != 2) {
		printf("Not enough arguments.\n");
		return ERROR_CODE;
	}
	

	/// Create scanf thread : check non-stop until getting "exit" command from the user.

	printf("Starting 7-boom Server!\n\n");
	p_thread_handle = CreateThreadSimple(inputThread, param, &p_thread_id); 

	/// Start running the server as usual.

	port = strtol(argv[1], NULL, 10); // "8888"
	retFromServer = MainServer(port); // initiate the programs' threads.

	wait_code = WaitForSingleObject( // Wait for IO thread to receive exit command and terminate
		p_thread_handle,			 // the handle object
		INFINITE);                   // Waiting forever for the theread to end - the worst case

	if (WAIT_TIMEOUT == wait_code)
	{
		printf("Timeout error when waiting\n");
		return ERROR_CODE;
	}
	else if (WAIT_FAILED == wait_code)
	{
		printf("WaitForSingleObjects has failed\n");
		return ERROR_CODE;
	}
	else if (WAIT_OBJECT_0 != wait_code)
	{
		printf("Error when waiting\n");
		return ERROR_CODE;
	}

	DWORD lpExitCode; //handle exit code
	GetExitCodeThread(p_thread_handle, &lpExitCode);
	if (lpExitCode == ERROR_CODE) {
		printf("Error with thread \n");
		return ERROR_CODE;
	}

	ret_val = CloseHandle(p_thread_handle);
	if (false == ret_val)
	{
		printf("Error when closing thread \n");
		return ERROR_CODE;
	}
	return  SUCCES_CODE + retFromServer;
}
