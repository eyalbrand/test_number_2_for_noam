// Initialize global variables.

#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include "global.h"

int ClosingAlert = 0;

int VERSUS_Couter = 0;
int user_Counter = 0;
int user_Counter_current = 0;
int op_num = 0;
int g_current_turn = 0;
int g_first_turn = 0;
int g_player_wait = 0;
int g_moved_taken = 0;
int g_first_switched = 0;
int g_wait_for_move = 0;
int g_player_loss = 0;
int g_start_turn_switch = 0;
int g_second_turn_switch = 0;
int g_second_game_ended = 0;
int g_send_menu = 0;
int g_send_second_menu = 0;
int g_send_ended = 0;
int g_player_boomed = 0;
int g_opponent_quit = 0;
int g_already_quit = 0;
int g_wrote_to_log = 0;