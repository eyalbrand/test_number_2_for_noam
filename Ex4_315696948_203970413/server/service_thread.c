
#include "MainServer.h"
#include "service_thread.h"
#include "global.h"

// A struct that holds the information about a subject
typedef struct user {
	char userName[MAX_LEN_OF_LINE];
	char move[MAX_LEN_OF_LINE];
} user;

user users_table[] = {
	{"a", "0"}, {"b", "0"}
};


/// <summary>
/// check if in the string theres a seven
/// </summary>
/// <param name="num_string">string that contains numbers</param>
/// <returns>return 1 or zero depends if there is a 7 or not</returns>
int contains_seven(char* num_string) {

	for (int i = 0; num_string[i] != '\0'; i++) {
		if (num_string[i] == '7') return 1;
	}
	return 0;

}


/// <summary>
/// checking if should boon or not
/// </summary>
/// <param name="last_move"> last player move </param>
/// <returns>return 1 or zero depends if should boom or not</returns>
int should_boom(int last_move) {
	int next_move = last_move + 1;
	char numStr[4]; // int takes 4 bytes in memory.
	sprintf(numStr, "%d", next_move);
	if ((next_move % 7 == 0) || (contains_seven(numStr))) return 1;
	return 0;
}


/// <summary>
/// reset the data on global perem
/// </summary>
void reset_data() {
	movesOfPlayers[g_current_turn] = 0;
	movesOfPlayers[1 - g_current_turn] = 0;
	g_current_turn = 0;
	g_wait_for_move = 0;
	g_wrote_to_log = 0;
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
		NULL,
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
/// checks if opponent has quit or not
/// </summary>
/// <param name="idx_current"></param>
/// <param name="t_socket">the socket</param>
/// <returns></returns>
int check_opponent_quit(int idx_current, SOCKET* t_socket) {
	TransferResult_t SendRes;
	char log_file_name[MAX_MESSAGE_TO_LOG] = { 0 };
	if (g_already_quit) return 2;
	if (g_opponent_quit) {
		g_opponent_quit = 0;
		char stropQuit[MAX_LEN_OF_MESSAGE];
		sprintf(stropQuit, "SERVER_OPPONENT_QUIT:%s\n", usernames[!idx_current]);
		sprintf(log_file_name, "Thread_log_%s.txt", usernames[idx_current]);
		write_to_log_file(log_file_name, RECIVED, CLIENT, "SERVER_OPPONENT_QUIT\n", MESSAGE_REGULER);
		SendRes = SendToClient(stropQuit, *t_socket);
		SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
		write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU", MESSAGE_REGULER);
		
		if (SendRes == TRNS_FAILED) {
			printf("Player disconnected. Exiting.");
			write_to_log_file(log_file_name, ERROR_MESSAGE, ERROR_MESSAGE, "Player disconnected. Exiting.\n", MESSAGE_ERROR_TYPE);
		}
		g_already_quit = 1;
		reset_data();
		return 1;
	}
	return 0;
}


/// <summary>
/// this is the function of each thread/client .this function deals with the cmmunication with the server, and the actions of each client.more over, it checks that there are not more than 2 clients connected.
/// </summary>
/// <param name="t_socket">the socket for each client</param>
/// <returns>if all went well it return 0, if not it return (-1)</returns>
DWORD service_thread_func(SOCKET* t_socket)
{
	int flag;
	int idx_current=-1;
	BOOL Done = FALSE;
	TransferResult_t SendRes;
	TransferResult_t RecvRes;
	char s[2] = ":";
	char message_to_log[MAX_MESSAGE_TO_LOG];
	char log_file_name[MAX_MESSAGE_TO_LOG];
	DWORD wait_code_request;
	HANDLE log_file;
	P_REQUEST_MUTEX = "REQUEST";
	P_GAME_S_MUTEX = "GAME_S";

	_mutex_REQUEST = CreateMutex(
		NULL,					/* default security attributes */
		FALSE,					/* initially not owned */
		P_REQUEST_MUTEX);	/* named mutex */

	if (NULL == _mutex_REQUEST){
		printf("Error when creating mutex: %d\n", GetLastError());
		return ERROR_CODE;
	}

	_mutex_GAME_S = CreateMutex(
		NULL,					/* default security attributes */
		FALSE,					/* initially not owned */
		P_GAME_S_MUTEX);	/* named mutex */

	if (NULL == _mutex_GAME_S){
		printf("Error when creating mutex: %d\n", GetLastError());
		return ERROR_CODE;
	}

	_mutex_BEGIN = CreateMutex(
		NULL,					/* default security attributes */
		FALSE,					/* initially not owned */
		P_BEGIN_MUTEX);	/* named mutex */

	if (NULL == _mutex_BEGIN){
		printf("Error when creating mutex: %d\n", GetLastError());
		return ERROR_CODE;
	}

	// main loop
	while ((!Done) && (!ClosingAlert)) { //done is private, closing alert is global
		char* AcceptedStr = NULL;							// this is our buffer
		RecvRes = ReceiveString(&AcceptedStr, *t_socket, idx_current);		// this is the respond from client

		if (RecvRes == TRNS_FAILED) {							//in case of failure
			if (!g_opponent_quit) {
				printf("Service socket error while reading, closing thread.\n");
			}
			closesocket(*t_socket);
			return ERROR_CODE;
		}
		else if (RecvRes == TRNS_DISCONNECTED){
			printf("Connection closed while reading, closing thread.\n");
			closesocket(*t_socket);
			return ERROR_CODE;
		}
		//now we check the client response and act accordignly
		//by AcceptedStr
		char* token;

		AcceptedStr[strlen(AcceptedStr)] = '\0';
		token = strtok(AcceptedStr, s);

		
		//first token is message_type
		if (STRINGS_ARE_EQUAL(token, "CLIENT_REQUEST")){
			token = strtok(NULL, s);
			sprintf(log_file_name, "Thread_log_%s.txt", token);
			log_file = create_file(log_file_name);
			CloseHandle(log_file);
			sprintf(message_to_log, "CLIENT_REQUEST:%s\n", token);
			write_to_log_file(log_file_name, RECIVED, CLIENT, message_to_log, MESSAGE_REGULER);

			if (NUM_OF_WORKER_THREADS == user_Counter_current) { // then we already have 2 players
				SendRes = SendToClient("SERVER_DENIED\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_DENIED\n", MESSAGE_REGULER);
				printf("Another client named %s tried to connect to the server but it's full\n\n", token);
				Done = true;
				goto DISCONNECT_CLIENT;
			}
			//mutex request
			wait_code_request = WaitForSingleObject(_mutex_REQUEST, INFINITE);
			if (WAIT_OBJECT_0 != wait_code_request){
				printf("Error when waiting for mutex\n");
				write_to_log_file(log_file_name, ERROR, ERROR, "Error when waiting for mutex\n", MESSAGE_ERROR_TYPE);
				ExitThread(ERROR_CODE);
			}

			else { //now we hold the parameters - in this case - USERNAME
				user_Counter_current++;
				user_Counter++;
				for (int i = 0; i < NUM_OF_WORKER_THREADS; i++){
					if (NULL == usernames[i])
					{
						idx_current = i;
						break;
					}
				}

				usernames[idx_current] = (char*)malloc(MAX_NAME_LEN); // hold the user name.
				if (usernames[idx_current] == NULL) {
					printf("Error allocating memory. exiting\n");
					write_to_log_file(log_file_name, ERROR, ERROR, "Error allocating memory. exiting\n", MESSAGE_ERROR_TYPE);
					exit(-1);
				}
				strcpy(usernames[idx_current], token);
				strcpy(users_table[idx_current].userName, token);
				printf("Client %s is connected to the server!\n\n", token);

				//release mutex request
				BOOL ret_val = ReleaseMutex(_mutex_REQUEST);
				if (FALSE == ret_val){
					printf("Error when releasing\n");
					write_to_log_file(log_file_name, ERROR, ERROR, "Error when releasing\n", MESSAGE_ERROR_TYPE);
					return ERROR_CODE;
				}
			}

			SendRes = SendToClient("SERVER_APPROVED\n", *t_socket);
			SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);

			write_to_log_file(log_file_name, SEND, SERVER, "SERVER_APPROVED\n", MESSAGE_REGULER);
			write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
		}
		//now we should return to the while and the client will make a decision
		
		if (STRINGS_ARE_EQUAL(token, "CLIENT_VERSUS")) { //check times
			write_to_log_file(log_file_name, RECIVED, CLIENT, "CLIENT_VERSUS\n", MESSAGE_REGULER);
			flag = VERSUS_F;
			VERSUS_Couter++;
			// set up timer until response from another user or timeout
			clock_t start = clock(), diff;
			while (TRUE) {
				diff = clock() - start;
				int msec = diff * 1000 / CLOCKS_PER_SEC;
				if (VERSUS_Couter % 2 == 0) {
					break;
				}
				if ((msec / 1000) > 30) {
					break;
				}
				

			}

			if ((VERSUS_Couter % 2 == 0 )) { // need 2 for a tango
				SendRes = SendToClient("GAME_STARTED\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "GAME_STARTED\n", MESSAGE_REGULER);
				// notify player 2 that's player 1 turn and vice versa.
				
				char* playerName, * oppName;
				playerName = usernames[g_current_turn];
				oppName = usernames[1 - g_current_turn];
				char turnSwitch[MAX_LEN_OF_MESSAGE];

				sprintf(turnSwitch, "TURN_SWITCH:%s\n", playerName);
				SendRes = SendToClient(turnSwitch, *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, turnSwitch, MESSAGE_REGULER);

				if (idx_current == g_first_turn) {
					SendRes = SendToClient("SERVER_MOVE_REQUEST\n", *t_socket);
					write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MOVE_REQUEST\n", MESSAGE_REGULER);
				}
				else {
					g_wait_for_move = 1;
					g_moved_taken = 0;
				}
			}
			else if (1 == VERSUS_Couter && op_num == 1){
				char stropQuit[MAX_LEN_OF_MESSAGE];
				sprintf(stropQuit, "SERVER_OPPONENT_QUIT:%s\n", usernames[!idx_current]);
				SendRes = SendToClient(stropQuit, *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_OPPONENT_QUIT\n", MESSAGE_REGULER);
				SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
				VERSUS_Couter = 0;
			}

			else{
				SendRes = SendToClient("SERVER_NO_OPPONENTS\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_NO_OPPONENTS\n", MESSAGE_REGULER);
				SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
				VERSUS_Couter = 0;
			}

			op_num = 0;
			token[0] = '\0';
		}

		if (STRINGS_ARE_EQUAL(token, "CLIENT_PLAYER_MOVE")){
			char strResultMessage[MAX_LEN_OF_LINE];
			char strFinishMessage[MAX_LEN_OF_LINE];
			char message_log[MAX_LEN_OF_LINE];
			int player_move;
			token = strtok(NULL, "\n");
			sprintf(message_log, "CLIENT_PLAYER_MOVE:%s\n", token);
			write_to_log_file(log_file_name, RECIVED, CLIENT, message_log, MESSAGE_REGULER);
			// player move: check the received string.
			if (token != NULL) {
				if (token[0] >= '0' && token[0] <= '9') { // then this is a number
					strcpy(users_table[idx_current].move, token);
					player_move = strtol(token, NULL, 10);
					movesOfPlayers[idx_current] = player_move;

					if ( contains_seven(token) || (player_move % 7 == 0)) { // player loss : example 3

						sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[idx_current], token);
						SendRes = SendToClient(strResultMessage, *t_socket);
						write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
						g_player_loss = 1;
						g_moved_taken = 1;
						while (g_player_loss) {
							if (g_send_ended) {
								sprintf(strFinishMessage, "GAME_ENDED:%s\n", usernames[1 - g_current_turn]);
								SendRes = SendToClient(strFinishMessage, *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER, strFinishMessage, MESSAGE_REGULER);
								g_second_game_ended = 1;
								g_send_ended = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						while (g_player_loss) {
							if (g_send_menu) {
								SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER,"SERVER_MAIN_MENU\n", MESSAGE_REGULER);
								g_send_second_menu = 1;
								g_send_menu = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						if (g_already_quit || check_opponent_quit(idx_current, t_socket) ==1);
						continue;						

					}
					else if (movesOfPlayers[idx_current] != (movesOfPlayers[1-idx_current] +1)) { 
						// player loss : example 2 - this wrong number
						sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[idx_current], token);
						write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
						SendRes = SendToClient(strResultMessage, *t_socket);
						g_player_loss = 1;
						g_moved_taken = 1;
						while (g_player_loss) {
							if (g_send_ended) {
								sprintf(strFinishMessage, "GAME_ENDED:%s\n", usernames[1 - g_current_turn]);
								SendRes = SendToClient(strFinishMessage, *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER, strFinishMessage, MESSAGE_REGULER);
								g_second_game_ended = 1;
								g_send_ended = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						while (g_player_loss) {
							if (g_send_menu) {
								SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
								g_send_second_menu = 1;
								g_send_menu = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						if (g_already_quit || check_opponent_quit(idx_current, t_socket) == 1);
						continue;
					}
					else { // the sent number is legal : continute the game.
					
						sprintf(strResultMessage, "GAME_VIEW:%s;%s;CONT\n", usernames[idx_current], token);
						SendRes = SendToClient(strResultMessage, *t_socket);
						write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
						
						/* start turn switch */
						g_wait_for_move = 1;
						g_moved_taken = 1;
						while (g_moved_taken) {
							if (g_start_turn_switch) {
								char* playerName;
								g_current_turn = 1 - g_current_turn;
								playerName = usernames[g_current_turn];
								char turnSwitch[MAX_LEN_OF_MESSAGE];
								sprintf(turnSwitch, "TURN_SWITCH:%s\n", playerName);
								SendRes = SendToClient(turnSwitch, *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER, turnSwitch, MESSAGE_REGULER);
								g_second_turn_switch = 1;
								g_first_switched = 0;
								g_moved_taken = 0;
								g_start_turn_switch = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						g_wait_for_move = 1;
						if (g_already_quit || check_opponent_quit(idx_current, t_socket) == 1);
					}
				}
				else if (STRINGS_ARE_EQUAL(token, "boom")) {
					int last_move = movesOfPlayers[1 - idx_current];
					
					if (should_boom(last_move)) {
						// Happy path
						g_player_boomed = 1;
						movesOfPlayers[idx_current] = movesOfPlayers[1 - idx_current] + 1;

						// map "boom" to its containing 7 integer. (7/14/17/21 ... ) 

						sprintf(strResultMessage, "GAME_VIEW:%s;%s;CONT\n", usernames[idx_current], token);
						SendRes = SendToClient(strResultMessage, *t_socket);
						write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);

						/* start turn switch */
						g_wait_for_move = 1;
						g_moved_taken = 1;
						while (g_moved_taken) {
							if (g_start_turn_switch) {
								char* playerName;
								g_current_turn = 1 - g_current_turn;
								playerName = usernames[g_current_turn];
								char turnSwitch[MAX_LEN_OF_MESSAGE];
								sprintf(turnSwitch, "TURN_SWITCH:%s\n", playerName);
								write_to_log_file(log_file_name, SEND, SERVER, turnSwitch, MESSAGE_REGULER); SendRes = SendToClient(turnSwitch, *t_socket);
								g_second_turn_switch = 1;
								g_first_switched = 0;
								g_moved_taken = 0;
								g_start_turn_switch = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						g_wait_for_move = 1;
						if (g_already_quit || check_opponent_quit(idx_current, t_socket) == 1);
					}
					else {
						// player loss : example 1
						sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[idx_current], token);
						SendRes = SendToClient(strResultMessage, *t_socket);
						write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
						g_player_boomed = 1;
						g_player_loss = 1;
						g_moved_taken = 1;
						while (g_player_loss) {
							if (g_send_ended) {
								sprintf(strFinishMessage, "GAME_ENDED:%s\n", usernames[1 - g_current_turn]);
								SendRes = SendToClient(strFinishMessage, *t_socket);
								write_to_log_file(log_file_name, SEND, SERVER, strFinishMessage, MESSAGE_REGULER);
								g_second_game_ended = 1;
								g_send_ended = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						while (g_player_loss) {
							if (g_send_menu) {
								SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
								
								write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
								g_send_second_menu = 1;
								g_send_menu = 0;
								break;
							}
							if (check_opponent_quit(idx_current, t_socket)) break;
						}
						if (g_already_quit || check_opponent_quit(idx_current, t_socket) == 1);
						continue;
					}
				}
				else { // unexpected input from client : cause loss.
				sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[idx_current], token);
				SendRes = SendToClient(strResultMessage, *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
				g_player_loss = 1;
				g_moved_taken = 1;
				while (g_player_loss) {
					if (g_send_ended) {
						sprintf(strFinishMessage, "GAME_ENDED:%s\n", usernames[1 - g_current_turn]);
						SendRes = SendToClient(strFinishMessage, *t_socket);
						write_to_log_file(log_file_name, SEND, SERVER, strFinishMessage, MESSAGE_REGULER);
						g_second_game_ended = 1;
						g_send_ended = 0;
						break;
					}
					if (check_opponent_quit(idx_current, t_socket)) break;
				}
				while (g_player_loss) {
					if (g_send_menu) {
						SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
						
						write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
						g_send_second_menu = 1;
						g_send_menu = 0;
						break;
					}
					if (check_opponent_quit(idx_current, t_socket)) break;
				}
				if (g_already_quit || check_opponent_quit(idx_current, t_socket) == 1);
				continue;
				}
			}

		}
		
		while (g_wait_for_move && idx_current == !g_current_turn) {
			char strResultMessage[MAX_LEN_OF_LINE];
			if (g_moved_taken) {

				if (g_player_boomed) {
					if (g_player_loss) {
						sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[!idx_current], "boom");
					}
					else {
						sprintf(strResultMessage, "GAME_VIEW:%s;%s;CONT\n", usernames[!idx_current], "boom");
					}
					g_player_boomed = 0;
				}
				else if (g_player_loss) {
					sprintf(strResultMessage, "GAME_VIEW:%s;%s;END\n", usernames[!idx_current], users_table[!idx_current].move);
				}
				else {
					sprintf(strResultMessage, "GAME_VIEW:%s;%s;CONT\n", usernames[!idx_current], users_table[!idx_current].move);
				}
				SendRes = SendToClient(strResultMessage, *t_socket);
				if (SendRes != TRNS_FAILED) {
					write_to_log_file(log_file_name, SEND, SERVER, strResultMessage, MESSAGE_REGULER);
				}
				if (SendRes == TRNS_FAILED) {
					
					printf("Player disconnected. Exiting.\n");
					g_wrote_to_log = 1;
					write_to_log_file(log_file_name, ERROR, ERROR, "Player disconnected. Exiting.\n", MESSAGE_ERROR_TYPE);
					free(usernames[idx_current]); //delete from usernames
					usernames[idx_current] = NULL;
					user_Counter_current--;
					Done = TRUE;
					g_opponent_quit = 1; // I quit :(
					break;
				}

				if (g_player_loss) { // the other player lost the game
					g_send_ended = 1;
					while (g_player_loss) {
						if (g_second_game_ended) {
							char strFinishMessage[MAX_LEN_OF_LINE];
							sprintf(strFinishMessage, "GAME_ENDED:%s\n", usernames[1-g_current_turn]);
							SendRes = SendToClient(strFinishMessage, *t_socket);
							write_to_log_file(log_file_name, SEND, SERVER, strFinishMessage, MESSAGE_REGULER);
							g_send_menu = 1;
							g_second_game_ended = 0;
						}
						if (g_send_second_menu) {
							SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
							
							write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
							g_send_second_menu = 0;
							g_player_loss = 0;
							reset_data(); // prepare the ground for another game
							break;
						}
					}
					break;
				}
				else { // continue the game regulary : send game view for player and start turn switch.

					g_start_turn_switch = 1;
					g_player_wait = 1;
					break;
				}
			}
			if (g_opponent_quit) {
				char stropQuit[MAX_LEN_OF_MESSAGE];
				sprintf(stropQuit, "SERVER_OPPONENT_QUIT:%s\n", usernames[!idx_current]);
				g_opponent_quit = 0;
				SendRes = SendToClient(stropQuit, *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_OPPONENT_QUIT\n", MESSAGE_REGULER);
				SendRes = SendToClient("SERVER_MAIN_MENU\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MAIN_MENU\n", MESSAGE_REGULER);
				reset_data();
				free(usernames[!idx_current]); //delete from usernames
				usernames[!idx_current] = NULL;
				user_Counter_current--;
				break;
			}
		}

		while (g_player_wait) {

			if (g_second_turn_switch) {
				char* playerName;
				playerName = usernames[g_current_turn];

				char turnSwitch[MAX_LEN_OF_MESSAGE];
				sprintf(turnSwitch, "TURN_SWITCH:%s\n", playerName);
				write_to_log_file(log_file_name, SEND, SERVER, turnSwitch, MESSAGE_REGULER);
				SendRes = SendToClient(turnSwitch, *t_socket);
				SendRes = SendToClient("SERVER_MOVE_REQUEST\n", *t_socket);
				write_to_log_file(log_file_name, SEND, SERVER, "SERVER_MOVE_REQUEST\n", MESSAGE_REGULER);
				g_player_wait = 0;
				g_second_turn_switch = 0;
				break;
			}
		}

		if (STRINGS_ARE_EQUAL(AcceptedStr, "CLIENT_DISCONNECT")){
			write_to_log_file(log_file_name, RECIVED, CLIENT, "CLIENT_DISCONNECT\n", MESSAGE_REGULER);
			printf("Player disconnected. Exiting.\n");
			write_to_log_file(log_file_name, ERROR, ERROR, "Player disconnected. Exiting.\n", MESSAGE_ERROR_TYPE);
			free(usernames[idx_current]);//delete from usernames
			usernames[idx_current] = NULL;
			user_Counter_current--;
			Done = TRUE;

		}
		
	DISCONNECT_CLIENT:
		free(AcceptedStr);
	}

	closesocket(*t_socket);
	return SUCCES_CODE;
}


/// <summary>
/// this function does tha actual information sending to the client.
/// </summary>
/// <param name="SendStr">a message which we want to sen</param>
/// <param name="t_socket">client socket.</param>
/// <returns>a kind of flag to check if the message has been sent.</returns>
TransferResult_t SendToClient(const char* SendStr, SOCKET t_socket)
{
	TransferResult_t SendRes = SendString(SendStr, t_socket);

	if (SendRes == TRNS_FAILED)
	{
		printf("Service socket error while writing, closing thread.\n");
		closesocket(t_socket);
		return SendRes;
	}

	Sleep(100); //wait for client to calculate
	return SendRes;
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
TransferResult_t ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd, int idx_current)
{
	char* CurPlacePtr = OutputBuffer;
	char log_file_name[MAX_MESSAGE_TO_LOG] = { 0 };
	char message_to_log[MAX_MESSAGE_TO_LOG] = { 0 };
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;

	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			if (WSAGetLastError() == 10054) {
				sprintf(log_file_name, "Thread_log_%s.txt", usernames[idx_current]);
				printf("Player disconnected. Exiting.\n");
				write_to_log_file(log_file_name, ERROR, ERROR, "Player disconnected. Exiting.\n", MESSAGE_ERROR_TYPE);
				free(usernames[idx_current]); //delete from usernames
				usernames[idx_current] = NULL;
				user_Counter_current--;
				g_opponent_quit = 1;
			}
			else {
				sprintf(log_file_name, "Thread_log_%s.txt", usernames[idx_current]);
				sprintf(message_to_log, "recv() function failed, error identifier %d\n", WSAGetLastError());
				printf("recv() function failed, error identifier %d\n", WSAGetLastError());
				write_to_log_file(log_file_name, ERROR, ERROR, message_to_log, MESSAGE_ERROR_TYPE);
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
TransferResult_t ReceiveString(char** OutputStrPtr, SOCKET sd, int idx_current)
{
	/* Recv the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	TransferResult_t RecvRes;
	char* StrBuffer = NULL;
	char log_file_name[MAX_MESSAGE_TO_LOG] = { 0 };
	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
		sprintf(log_file_name, "Thread_log_%s.txt", usernames[idx_current]);
		write_to_log_file(log_file_name, ERROR, ERROR, "The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n", MESSAGE_ERROR_TYPE);
		return TRNS_FAILED;
	}

	/* The request is received in two parts. First the Length of the string (stored in
	   an int variable ), then the string itself. */

	RecvRes = ReceiveBuffer(
		(char*)(&TotalStringSizeInBytes),
		(int)(sizeof(TotalStringSizeInBytes)), // 4 bytes
		sd, idx_current);

	if (RecvRes != TRNS_SUCCEEDED) return RecvRes;

	StrBuffer = (char*)malloc(TotalStringSizeInBytes * sizeof(char));

	if (StrBuffer == NULL)
		return TRNS_FAILED;

	RecvRes = ReceiveBuffer(
		(char*)(StrBuffer),
		(int)(TotalStringSizeInBytes),
		sd, idx_current);

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