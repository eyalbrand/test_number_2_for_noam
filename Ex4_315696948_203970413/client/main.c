/*
Authors - Eyal Brand 203970413
		  Ori Noy 315696948
Project: Client
Description: client project as part of Server-Client project that handels a game of "7 boom".
handleing the the game from user side - sending and recviving messages from server from first connection until
the user deside to leave the game/system
*/


#include "RunClient.h"

/// <summary>
/// main function of the client
/// </summary>
/// <param name="argc"> amount of args given to the CMD</param>
/// <param name="argv"> the arguments themself given to the CMD</param>
/// <returns> client process fails or not </returns>
int main(int argc, char* argv[]) {
	int valid = 0;
	char message_to_log[MAX_MESSAGE_TO_LOG] = { 0 };
	TransferResult_t SendRes;
	HANDLE log_file;

	if (argc != 4)
	{
		printf("IO ERROR! not enough inputs\n");
		return -1;
	}

	sprintf(SERVER_ADDRESS_STR, argv[1]); //  server address "127.0.0.1"
	sprintf(SERVER_PORT_STR, argv[2]);    //  server port string "8888"
	SERVER_PORT = atoi(SERVER_PORT_STR);  // switch server port to int
	sprintf(user_name, argv[3]);		  //  user name

	/// creating log file for client
	sprintf(log_file_name, "Client_log_%s.txt", user_name);
	log_file = create_file(log_file_name);
	CloseHandle(log_file);

	/// creating first connection to server
	valid = create_connection();
	if (valid == USER_WANT_OUT)
	{
		return 0;
	}
	else if (valid != 0)
		return valid;

	/// main Client function - complete conversation between the Client and server after first connection
	valid = RecvData();

	/// if user or done playing - getting out gentlly
	if (valid == USER_WANT_OUT) {
		if (g_server_disconnection) {
			closesocket(m_socket);
			WSACleanup();
			return 0;
		}
		SendRes = SendString("CLIENT_DISCONNECT", m_socket);
		write_to_log_file(log_file_name, sent, client, "CLIENT_DISCONNECT\n", MESSAGE_REGULER);

		if (SendRes == TRNS_FAILED) {
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else {
			closesocket(m_socket);
			WSACleanup();
			return 0;
		}
	}
	else {
		return valid;
	}
}


/// <summary>
/// This function creates a handle that responsible for creating new file.
/// </summary>
/// <param name="file_name">file name</param>
/// <returns>Handle object</returns>
HANDLE create_file(char* file_name) {
	HANDLE hfile = CreateFileA(
		file_name,
		GENERIC_WRITE,
		2,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hfile == INVALID_HANDLE_VALUE) {
		printf("Terminal failure: Unable to create file %s for write.", file_name);
		CloseHandle(hfile);
		exit(1); //
	}
	return hfile;
}


/// <summary>
/// uses a socket to send a buffer.
/// </summary>
/// <param name="Buffer">the buffer containing the data to be sent</param>
/// <param name="BytesToSend">the number of bytes from the Buffer to send</param>
/// <param name="sd">the socket used for communication</param>
/// <returns>TRNS_SUCCEEDED - if sending succeeded; TRNS_FAILED - otherwise;</returns>
TransferResult_t SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;

	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			if (g_user_choosing != -1) { // then the server abruptly disconnected
				return 3;
			}
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}

		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}


/// <summary>
/// uses a socket to send a string
/// </summary>
/// <param name="Str">the string to send</param>
/// <param name="sd">the socket used for communication</param>
/// <returns></returns>
TransferResult_t SendString(const char* Str, SOCKET sd)
{
	/* Send the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t SendRes;

	/* The request is sent in two parts. First the Length of the string (stored in
	  an int variable ), then the string itself. */
	TotalStringSizeInBytes = (int)(strlen(Str) + 1); // terminating zero also sent

	SendRes = SendBuffer(
		(const char*)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // sizeof(int)
		sd);

	if (SendRes != TRNS_SUCCEEDED) return SendRes;

	SendRes = SendBuffer(
		(const char*)(Str),
		(int)(TotalStringSizeInBytes),
		sd);

	return SendRes;
}


/// <summary>
/// uses a socket to receive a buffer
/// </summary>
/// <param name="OutputBuffer">pointer to a buffer into which data will be written</param>
/// <param name="BytesToReceive">output parameter. if function returns TRNS_SUCCEEDED, then this will point at an int containing the number of bytes received</param>
/// <param name="sd">the socket used for communication</param>
/// <returns>TRNS_SUCCEEDED - if receiving succeeded; TRNS_DISCONNECTED - if the socket was disconnected; TRNS_FAILED - otherwise</returns>
TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			if (!(ERROR_SERVER_DISCONNECT == WSAGetLastError())) {
				printf("recv() failed, error %d\n", WSAGetLastError());
			}
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}


/// <summary>
/// uses a socket to receive a string, and stores it in dynamic memory.
/// </summary>
/// <param name="OutputStrPtr">a pointer to a char-pointer that is initialized to NULL</param>
/// <param name="sd">the socket used for communication</param>
/// <returns>TRNS_SUCCEEDED - if receiving and memory allocation succeeded; TRNS_DISCONNECTED - if the socket was disconnected; TRNS_FAILED - otherwise</returns>
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd)
{
	/* Recv the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;

	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be a pointer to a char pointer that is initialized to NULL\n");
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in
	  an int variable ), then the string itself. */

	RecvRes = ReceiveBuffer(
		(char*)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // 4 bytes
		sd);

	if (RecvRes != TRNS_SUCCEEDED) return RecvRes;

	StrBuffer = (char*)malloc(TotalStringSizeInBytes * sizeof(char));

	if (StrBuffer == NULL)
		return TRNS_FAILED;

	RecvRes = ReceiveBuffer(
		(char*)(StrBuffer),
		(int)(TotalStringSizeInBytes),
		sd);

	if (RecvRes == TRNS_SUCCEEDED)
	{
		*OutputStrPtr = StrBuffer;
	}
	else
	{
		free(StrBuffer);
	}

	return RecvRes;
}


/// <summary>
/// create first connection to server
/// </summary>
/// <returns> returns if connection succeed or not. also if user decided to exit</returns>
int create_connection()
{

	TransferResult_t SendRes;
	char user_name_rqeuest[MAX_ORDER + MAX_USER_NAME], user_name_error[MAX_ORDER + MAX_USER_NAME], user_request[MAX_ORDER + MAX_USER_NAME];
	int valid, res_of_scanf;

	// Initialize Winsock.
	WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.
	//Call WSAStartup and check for errors.

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	// Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.
	// Create a socket.
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	if (m_socket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return -1; // return the correct value?
	}
	//Create a sockaddr_in object clientService and set  values.
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr(SERVER_ADDRESS_STR); //Setting the IP address to connect to
	clientService.sin_port = htons(SERVER_PORT); //Setting the port to connect to.

	while (1) {

		valid = connect(m_socket, (SOCKADDR*)&clientService, sizeof(clientService));
		// Call the connect function, passing the created socket and the sockaddr_in structure as parameters
		if (valid == SOCKET_ERROR) {
			printf("Failed connecting to server on %s:%d.\n", SERVER_ADDRESS_STR, SERVER_PORT);
			sprintf(user_name_error, "Failed connecting to server on %s:%d.\n", SERVER_ADDRESS_STR, SERVER_PORT);
			write_to_log_file(log_file_name, error, error, user_name_error, MESSAGE_ERROR_TYPE);

			while (1) { // try to reconnect or get out
				printf("Choose what to do next:\n1. Try to reconnect\n2. Exit\n");
				char temp[MAX_MESSAGE_TO_LOG] = { 0 };
				res_of_scanf = scanf("%s", &temp);
				if (strlen(temp) > 1) { //invalid value
					printf("Error: Illegal command\n");
					write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
					continue;
				}
				g_user_choosing = atoi(temp);
				if (g_user_choosing == 1) {
					break;
				}
				else if (g_user_choosing == 2) {
					return USER_WANT_OUT;
				}
				else { //invalid value
					printf("Error: Illegal command\n");
					write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
					res_of_scanf = 1;
					continue;
				}
			}

		}
		else
			break;
	}
	// if we here its means we connect to server
	sprintf(user_request, "CLIENT_REQUEST:%s", user_name);
	sprintf(user_name_rqeuest, "CLIENT_REQUEST:%s\n", user_name);
	write_to_log_file(log_file_name, sent, client, user_name_rqeuest, MESSAGE_REGULER);
	SendRes = SendString(user_request, m_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}

	return 0;
}


/// <summary>
/// 
/// </summary>
/// <returns></returns>
int connection_lost() {
	int valid = 0;
	printf("Server disconnected. Exiting.\n");
	return USER_WANT_OUT;
}


/// <summary>
/// iploading main menu before starting a game
/// </summary>
/// <returns>user decision</returns>
int server_main_menu() {
	int valid = 0, res_of_scanf;
	TransferResult_t SendRes;
	while (1) {
		printf("Choose what to do next:\n 1. Play against another client\n 2. Quit\n");
		char temp[MAX_MESSAGE_TO_LOG] = { 0 };
		res_of_scanf = scanf("%s", &temp);

		if (strlen(temp) > 1) { //invalid value
			printf("Error: Illegal command\n");
			write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
			continue;
		}
		g_user_choosing = atoi(temp);

		if (g_user_choosing == 1) {
			SendRes = SendString("CLIENT_VERSUS", m_socket);
			write_to_log_file(log_file_name, sent, client, "CLIENT_VERSUS\n", MESSAGE_REGULER);
			if (SendRes == TRNS_SUCCEEDED) { // send success
				break;
			}
		}
		else if (g_user_choosing == 2) {
			SendRes = SendString("CLIENT_DISCONNECT", m_socket);
			valid = USER_WANT_OUT;
			break;
		}
		else { //invalid value
			printf("Error: Illegal command\n");
			write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
			res_of_scanf = 1;
			continue;
		}

		if (SendRes == TRNS_FAILED)
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if (SendRes == 3) { // then the server abruptly disconnected
			// write to log file
			printf("Server disconnected. Exiting.\n");
			write_to_log_file(log_file_name, error, error, "Server disconnected. Exiting.\n", MESSAGE_ERROR_TYPE);
			g_server_disconnection = 1;
			valid = USER_WANT_OUT;
			break;
		}
	}
	return valid;
}


/// <summary>
/// getting request from server to play a move in the game
/// </summary>
/// <returns>the player choosen move</returns>
int server_player_move_request() {
	TransferResult_t SendRes;
	int res_of_scanf;
	char user_move[MAX_USER_MOVE] = { 0 }, user_move_message[MAX_USER_MOVE + MAX_ORDER] = { 0 };
	while (1)
	{
		printf("Enter the next number or boom \n");
		res_of_scanf = scanf("%s", user_move);
		if (STRINGS_ARE_EQUAL("boom", user_move))
		{
			sprintf(user_move_message, "CLIENT_PLAYER_MOVE:%s\n", user_move);
			write_to_log_file(log_file_name, sent, client, user_move_message, MESSAGE_REGULER);
			break;
		}
		if (user_move[0] >= '0' && user_move[0] <= '9') {
			sprintf(user_move_message, "CLIENT_PLAYER_MOVE:%s\n", user_move);
			write_to_log_file(log_file_name, sent, client, user_move_message, MESSAGE_REGULER);
			break;
		}
		else
			printf("Error: Illegal command\n");
		write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);

	}
	SendRes = SendString(user_move_message, m_socket);
	if (SendRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}
	return 0;
}


/// <summary>
/// game view after last player move
/// </summary>
/// <param name="message"> the message that was recivied from server</param>
/// <returns></returns>
int server_game_view(char message[]) {
	write_to_log_file(log_file_name, recivied, server, message, MESSAGE_REGULER);
	char* token;
	char player_name[MAX_USER_NAME] = { 0 }, player_move[MAX_USER_MOVE] = { 0 }, game_status[MAX_USER_MOVE] = { 0 };
	token = strtok(message, ":");
	token = strtok(NULL, ";"); // player's name.
	strcpy(player_name, token);

	token = strtok(NULL, ";"); // player's move.
	strcpy(player_move, token);

	token = strtok(NULL, "\n"); // server's message.
	strcpy(game_status, token);

	printf("%s move was %s\n%s\n", player_name, player_move, game_status);
	return 0;
}


/// <summary>
/// parsing the game ended message
/// </summary>
/// <param name="server_game_ended_message"> message form server</param>
/// <returns></returns>
int server_game_ended(char* server_game_ended_message) {
	write_to_log_file(log_file_name, recivied, server, server_game_ended_message, MESSAGE_REGULER);
	char* token;
	token = strtok(server_game_ended_message, ":");
	token = strtok(NULL, "\n"); // player's name.
	printf("%s won!\n", token);
	return 0;
}


/// <summary>
/// parsing the game started message from server
/// </summary>
/// <returns></returns>
int server_game_started() {
	char send_message[MAX_MESSAGE_TO_LOG] = { 0 };
	sprintf(send_message, "GAME_STARTED\n");
	write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
	printf("Game is on!\n");
	return 0;
}


/// <summary>
/// parsing the message server approved from server
/// </summary>
/// <returns></returns>
int server_approved() {
	char send_message[MAX_MESSAGE_TO_LOG] = { 0 };
	printf("Connected to server on %s:%d\n", SERVER_ADDRESS_STR, SERVER_PORT);
	sprintf(send_message, "SERVER_APPROVED\nConnected to server on %s:%d\n", SERVER_ADDRESS_STR, SERVER_PORT);
	write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
	return 0;
}


/// <summary>
/// parsing the message server denied from server and user deciding if try again or get out
/// </summary>
/// <returns></returns>
int server_denied() {
	int valid, res_of_scanf;
	char send_message[MAX_MESSAGE_TO_LOG] = { 0 };
	closesocket(m_socket);
	WSACleanup();

	printf("Server on %s:%d denied the connection request.\n", SERVER_ADDRESS_STR, SERVER_PORT);
	sprintf(send_message, "Server on %s:%s denied the connection request\n", SERVER_ADDRESS_STR, SERVER_PORT_STR);
	write_to_log_file(log_file_name, error, error, send_message, MESSAGE_ERROR_TYPE);
	while (1) {
		printf("Choose what to do next:\n1. Try to reconnect\n2. Exit\n");
		char temp[MAX_MESSAGE_TO_LOG] = { 0 };
		res_of_scanf = scanf("%s", &temp);
		if (strlen(temp) > 1) { //invalid value
			printf("Error: Illegal command\n");
			write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
			continue;
		}

		g_user_choosing = atoi(temp);
		if (g_user_choosing == 1) {
			valid = create_connection();
			return valid;
		}
		else if (g_user_choosing == 2) {
			return USER_WANT_OUT;

		}
		else { //invalid value
			printf("Error: Illegal command\n");
			write_to_log_file(log_file_name, error, error, "Error: Illegal command\n", MESSAGE_ERROR_TYPE);
			res_of_scanf = 1;
			continue;
		}
	}

}


/// <summary>
/// parsing message no opponent from server
/// </summary>
/// <returns></returns>
int server_no_opponents() {
	char send_message[MAX_MESSAGE_TO_LOG] = { 0 };
	sprintf(send_message, "SERVER_NO_OPPONENTS\n");
	write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
	printf("No opponent was found\n");
	return 0;
}


/// <summary>
/// parsing message opponent quit from server
/// </summary>
/// <returns></returns>
int server_opponents_quit() {
	char send_message[100] = { 0 };
	sprintf(send_message, "SERVER_OPPONENT_QUIT\n");
	write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
	printf("Opponent quit .\n");
	return 0;
}


/// <summary>
/// parsing and handeling turn_switch from server
/// </summary>
/// <param name="p"> message from server</param>
/// <returns></returns>
int server_turn_switch(p) {
	char send_message[MAX_MESSAGE_TO_LOG] = { 0 };
	char* token;
	token = strtok(p, ":");
	while (token != NULL) {
		token = strtok(NULL, "\n");
		break;
	}
	if (STRINGS_ARE_EQUAL(token, user_name)) {
		sprintf(send_message, "TURN_SWITCH:Your turn!\n");
		write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
		printf("Your turn!\n");
		return 0;
	}
	else {
		sprintf(send_message, "TURN_SWITCH:%s's turn!\n", token);
		write_to_log_file(log_file_name, recivied, server, send_message, MESSAGE_REGULER);
		printf("%s's turn!\n", token);
		return 0;
	}

}


/// <summary>
/// getting message from server and understanding which message is it and how to handle it
/// </summary>
/// <param name="order">persing the hall message into just first string until ":"</param>
/// <param name="p"> the hall message that was recivied vrom server</param>
/// <returns>0 if all went ok, 1 if not (or if user wants to get out)</returns>
int call_the_order(char order[], char p[]) {  // return valid = 0 if everything went well
	int valid = 1;
	if (STRINGS_ARE_EQUAL(order, "SERVER_MAIN_MENU")) {
		write_to_log_file(log_file_name, recivied, server, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
		valid = server_main_menu();
	}

	else if (STRINGS_ARE_EQUAL(order, "SERVER_MOVE_REQUEST")) {
		write_to_log_file(log_file_name, recivied, server, "SERVER_MOVE_REQUEST\n", MESSAGE_REGULER);
		valid = server_player_move_request();
	}

	else if (STRINGS_ARE_EQUAL(order, "GAME_ENDED"))
		valid = server_game_ended(p);

	else if (STRINGS_ARE_EQUAL(order, "GAME_VIEW"))
		valid = server_game_view(p);

	else if (STRINGS_ARE_EQUAL(order, "SERVER_APPROVED"))
		valid = server_approved();

	else if (STRINGS_ARE_EQUAL(order, "SERVER_DENIED"))
		valid = server_denied();

	else if (STRINGS_ARE_EQUAL(order, "GAME_STARTED"))
		valid = server_game_started();

	else if (STRINGS_ARE_EQUAL(order, "SERVER_NO_OPPONENTS"))
		valid = server_no_opponents();

	else if (STRINGS_ARE_EQUAL(order, "SERVER_OPPONENT_QUIT"))
		valid = server_opponents_quit();

	else if (STRINGS_ARE_EQUAL(order, "TURN_SWITCH"))
		valid = server_turn_switch(p);

	return valid;
}


/// <summary>
/// parsing the message from server into just the first string until ":"
/// </summary>
/// <param name="p"> the message from server </param>
/// <returns> returning the value that we gor form call the order function</returns>
int get_order_from_message(char p[]) {
	char order[MAX_ORDER];
	int i = 0, valid = 0;
	if (p == NULL)
	{
		return 0;
	}
	while (p[i] != '\n' && p[i] != ':') {
		order[i] = p[i];
		i++;
	}
	order[i] = '\0';
	valid = call_the_order(order, p); // return 0 if everything went well

	return valid;
}


/// <summary>
/// the main finction of the client script. open all the time after connecting to recivie messages from server and handeling them
/// </summary>
/// <returns></returns>
int RecvData()
{
	int valid = 0;
	TransferResult_t RecvRes;
	while (1)
	{
		char* AcceptedStr = NULL;
		RecvRes = ReceiveString(&AcceptedStr, m_socket);

		if (RecvRes == TRNS_FAILED)
		{
			printf("Server disconnected. Exiting\n");
			write_to_log_file(log_file_name, error, error, "Server disconnected. Exiting\n", MESSAGE_ERROR_TYPE);
			return 0x555;
		}
		else if (RecvRes == TRNS_DISCONNECTED)
		{
			connection_lost(); // do we need to upgrade this func?
		}
		else
		{
			valid = get_order_from_message(AcceptedStr);
			if (valid != 0)
				break;

		}

		free(AcceptedStr);
	}

	return valid;
}