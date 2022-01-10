#pragma warning(disable: 6387)

#pragma once
//This Header file contains all of includes which are being used in main.c


#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <math.h>
#include <crtdbg.h>
#include <string.h>
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>

#include "FileHandling.h"
#include "ProcessHandling.h"

//server messages
#define SERVER_APPROVED 0
#define SERVER_DENIED 1
#define SERVER_MAIN_MENU 2
#define GAME_STARTED 3
#define TURN_SWITCH 4
#define SERVER_MOVE_REQUEST 5
#define GAME_ENDED 6
#define SERVER_NO_OPPONENTS 7
#define GAME_VIEW 8
#define SERVER_OPPONENT_QUIT 9 

//client messages
#define CLIENT_REQUEST 0
#define CLIENT_VERSUS 1
#define CLIENT_PLAYER_MOVE 2
#define CLIENT_DISCONNECT 3

//receive options
#define TRNS_FAILED 0
#define TRNS_DISCONNECTED 1
#define TRNS_SUCCEEDED 2
#define TRNS_TIMEOUT 3
#define TIME_OUT_ERROR 0

// general
#define MAX_SOCKETS 3
#define CHUNK 200
#define READ 0
#define WRITE 1
#define MAX_LOG_MESSAGE 50
#define SOCKET_ERR 0
#define WINAPI_ERR 1
#define MISC_ERR 2
#define SENT 0
#define RECEIVED 1

//structures
typedef struct Game
{
	char* players[MAX_SOCKETS]; //max: two players playing + one player being denied
	int Game_number;
	int game_ended;
	int game_started;
	int players_num;
}Game;