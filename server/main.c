#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include "ProcessHandling.h"
#include "HardCodedData.h"

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
typedef struct Game
{
	char* players[MAX_SOCKETS]; //max: two players playing + one player being denied
	int Game_number;
	int game_ended;
	int players_num;
}Game;

Game game_state = { 0 };
SOCKET allSockets[MAX_SOCKETS];
int socketCount = 0;
HANDLE GlobalSemaphore;
HANDLE GameManagerHandle;
HANDLE GameMutex;

int  ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
{
	char* CurPlacePtr = OutputBuffer;
	int BytesJustTransferred;
	int RemainingBytesToReceive = BytesToReceive;
	int Last_Error_Value = 0;
	
	while (RemainingBytesToReceive > 0)
	{
		/* send does not guarantee that the entire message is sent */
		
		BytesJustTransferred = recv(sd, CurPlacePtr, RemainingBytesToReceive, 0);
		if (BytesJustTransferred == SOCKET_ERROR)
		{
			Last_Error_Value = WSAGetLastError();
			if (Last_Error_Value == TIME_OUT_ERROR) {
				return TRNS_TIMEOUT;
			}
			printf("recv() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}
		else if (BytesJustTransferred == 0)
			return TRNS_DISCONNECTED; // recv() returns zero if connection was gracefully disconnected.

		RemainingBytesToReceive -= BytesJustTransferred;
		CurPlacePtr += BytesJustTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

int ReceiveString(char** OutputStrPtr, SOCKET sd)
{
	/* Recv the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	int RecvRes;
	char* StrBuffer = NULL;

	if ((OutputStrPtr == NULL) || (*OutputStrPtr != NULL))
	{
		printf("The first input to ReceiveString() must be "
			"a pointer to a char pointer that is initialized to NULL. For example:\n"
			"\tchar* Buffer = NULL;\n"
			"\tReceiveString( &Buffer, ___ )\n");
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

int RecvDataThread(char** AcceptedStr, SOCKET client_s)
{
	int RecvRes;
	RecvRes = ReceiveString(AcceptedStr, client_s);

	if (RecvRes == TRNS_FAILED)
	{
		printf("Socket error while trying to write data to socket\n");
		return TRNS_FAILED;
		//return 0x555;
	}
	else if (RecvRes == TRNS_DISCONNECTED)
	{
		printf("Server closed connection. Bye!\n");    /////MAKES INFINITE LOOOP
		return TRNS_DISCONNECTED;
		//return 0x555;
	}
	else if (RecvRes == TRNS_TIMEOUT) {
		printf("Server timedout. Bye!\n");
		return TRNS_TIMEOUT;
		//return 0x555;
	}
	return TRNS_SUCCEEDED;
}

char* PrepareMessage(int messageType, char* arg1, char* arg2, char* arg3) // need to reduce lines------------------
{
	char* buffer;
	int buffSize;
	switch (messageType)
	{
	case SERVER_APPROVED:
		if ((buffSize = snprintf(NULL, 0, "SERVER_APPROVED\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_APPROVED\n");
		break;
	case SERVER_DENIED:
		if ((buffSize = snprintf(NULL, 0, "SERVER_DENIED\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_DENIED\n");
		break;
	case SERVER_MAIN_MENU:
		if ((buffSize = snprintf(NULL, 0, "SERVER_MAIN_MENU:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_MAIN_MENU\n");
		break;
	case GAME_STARTED:
		if ((buffSize = snprintf(NULL, 0, "GAME_STARTED\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "GAME_STARTED\n");
		break;
	case TURN_SWITCH:
		if ((buffSize = snprintf(NULL, 0, "TURN_SWITCH:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "TURN_SWITCH:%s\n", arg1);
		break;
	case SERVER_MOVE_REQUEST:
		if ((buffSize = snprintf(NULL, 0, "SERVER_MOVE_REQUEST\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_MOVE_REQUEST\n");
		break;
	case GAME_ENDED:
		if ((buffSize = snprintf(NULL, 0, "GAME_ENDED:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "GAME_ENDED:%s\n", arg1);
		break;
	case SERVER_NO_OPPONENTS:
		if ((buffSize = snprintf(NULL, 0, "SERVER_NO_OPPONENTS\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_NO_OPPONENTS\n");
		break;
	case GAME_VIEW:
		if ((buffSize = snprintf(NULL, 0, "GAME_VIEW:%s;%s;%s\n", arg1, arg2, arg3)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;  
		}
		sprintf_s(buffer, buffSize + 1, "GAME_VIEW:%s;%s;%s\n", arg1, arg2, arg3);
		break;
	case SERVER_OPPONENT_QUIT:
		if ((buffSize = snprintf(NULL, 0, "SERVER_OPPONENT_QUIT:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_OPPONENT_QUIT\n");
		break;
	default:
		buffer = NULL;
	}
	return buffer;
}

int SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd)
{
	const char* CurPlacePtr = Buffer;
	int BytesTransferred;
	int RemainingBytesToSend = BytesToSend;
	int Last_Error_Value;
	while (RemainingBytesToSend > 0)
	{
		/* send does not guarantee that the entire message is sent */
		BytesTransferred = send(sd, CurPlacePtr, RemainingBytesToSend, 0);
		if (BytesTransferred == SOCKET_ERROR)
		{
			Last_Error_Value = WSAGetLastError();
			if (Last_Error_Value == TIME_OUT_ERROR) {
				return TRNS_TIMEOUT;
			}
			printf("send() failed, error %d\n", WSAGetLastError());
			return TRNS_FAILED;
		}

		RemainingBytesToSend -= BytesTransferred;
		CurPlacePtr += BytesTransferred; // <ISP> pointer arithmetic
	}

	return TRNS_SUCCEEDED;
}

int SendString(const char* Str, SOCKET sd)
{
	//DWORD timeout = 15;
	//if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof timeout) < 0)
	//	printf("setsockopt failed\n");

	/* Send the the request to the server on socket sd */
	int TotalStringSizeInBytes;
	int SendRes;

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

int containsDigit(int number) // gets str and checks there is no 7 in it or 7 doesnt divide the number
{
	if (number == 0)
		return 0;
	if (number % 7 == 0)
		return 1;
	int curr_digit;
	while (number != 0)
	{
		curr_digit = number % 10;
		if (curr_digit == 7) {
			return 1;
		}
		number /= 10;
	}
	return 0;
}

int containsBOOM(char* input) {
	char BOOM[5] = "boom";
	if (!strcmp(BOOM, input)) {
		return 1; // if boom was the input 
	}
	return 0;
}

int LEGAL_MOVE(char* next_move) {
	//int legal_move = Game_number+1;
	//if (containsDigit(legal_move)) {
	//	legal_move = -1;
	//}
	//return legal_move;
	if (containsBOOM(next_move)) {
		if (containsDigit(game_state.Game_number +1)) {// add mutex to protect shared structure - game_status
			return 1;
		}
		return 0;
	}
	else {
		if (containsDigit(game_state.Game_number + 1)) { // add mutex to protect shared structure - game_status
			return 0;
		}
		if (atoi(next_move) == game_state.Game_number + 1) {// add mutex to protect shared structure - game_status
			return 1;
		}
	}
	printf("FORGOT ssssssss");
	return 0;
}

int getArgsFromMessage(char* message, char** arg1)
{
	char mType[20] = { 0 };
	int i = 0; 
	char* temp = message, *temp1;
	while (temp != NULL && *temp != ':')
	{
		mType[i] = *temp;
		temp++;
		i++;
	}
	if (temp == NULL)
		return -1;
	mType[i] = '\0';
	*temp = '\0';
	if (!strcmp(mType, "CLIENT_REQUEST")) {
		temp1 = strchr(temp + 1, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);    ////////////
		return CLIENT_REQUEST;
	}
	if (!strcmp(mType, "CLIENT_PLAYER_MOVE")) {
		temp1 = strchr(temp + 1, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);    //////////// WORKed AFTER I USED MALLOC FOR NEXT_MOVE  NEED TO FREE
		return CLIENT_PLAYER_MOVE;
	}
}

int transMessageToInt(char* message, char** arg1)
{
	if (!strcmp(message, "CLIENT_VERSUS\n"))
		return CLIENT_VERSUS;
	if (!strcmp(message, "CLIENT_DISCONNECT\n"))
		return CLIENT_DISCONNECT;
	//with arguments:
	return getArgsFromMessage(message, arg1);
}

int createAndSendMessage(SOCKET client_s, int messageType, char* arg1, char* arg2, char* arg3)
{
	char* send;
	send = PrepareMessage(messageType, arg1, arg2, arg3);
	if (SendString(send, client_s) != TRNS_SUCCEEDED) {
		printf("Failed to send Server approved");
		return 1;
	}
	free(send);
	return 0;
}

DWORD WINAPI threadExecute(SOCKET *client_s)
{
	Sleep(0);
	/*
	struct timeval timeout;
	timeout.tv_sec = 15;
	timeout.tv_usec = 0;

	if (setsockopt(*client_s, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof timeout) < 0)
		printf("setsockopt failed\n");

	if (setsockopt(*client_s, SOL_SOCKET, SO_SNDTIMEO, &timeout,sizeof timeout) < 0)
		printf("setsockopt failed\n");
	*/
	func(client_s, game_state.players_num);
}

int decideTurn()
{
	return game_state.Game_number % 2;
}

int func(SOCKET *client_s, int index)
{
	char * received = NULL, *arg;
	int Rec, type, threadTurn, canStartGame;
	if ((arg = malloc(20 * sizeof(char))) == NULL)
		return 1;
	DWORD timeout = 15000;

	while (!game_state.game_ended)
	{
		if (setsockopt(*client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
			printf("setsockopt failed\n");
		Rec = RecvDataThread(&received, *client_s);
		if (Rec != TRNS_SUCCEEDED) {
			continue;
			//free(received);
			//return 1;
		}
		type = transMessageToInt(received, &arg);
		free(received);
		received = NULL;
		switch (type)
		{
		case CLIENT_REQUEST:
			//---------------add mutex before approaching game_state -------------------------------
			if (game_state.players_num >= 2)
			{
				//server denied
				createAndSendMessage(*client_s, SERVER_DENIED, NULL, NULL, NULL);
				Sleep(1000);
				return 0;
			}
			//server accepted
			createAndSendMessage(*client_s, SERVER_APPROVED, NULL, NULL, NULL);

			//add player's name to game struct
			game_state.players[game_state.players_num] = malloc(20 * sizeof(char));
			strcpy(game_state.players[index], arg);

			//inc players num 
			game_state.players_num++;

			//send MAIN MENU
			createAndSendMessage(*client_s, SERVER_MAIN_MENU, NULL, NULL, NULL);
			break;
		case CLIENT_VERSUS:
			//wait for other player to join (15 sec)
			if (game_state.players_num < 2) { // if this is the first player- wait forever for the second one
				WaitForSingleObject(GameManagerHandle, 15000);
			}
			if (game_state.players_num >= 2)
			{
				SetEvent(GameManagerHandle);
			}
			canStartGame = (game_state.players_num >= 2);
			if (!canStartGame)
			{
				createAndSendMessage(*client_s, SERVER_NO_OPPONENTS, NULL, NULL, NULL);
				createAndSendMessage(*client_s, SERVER_MAIN_MENU, NULL, NULL, NULL);
				continue;
			}
			// --------------- need to add mutex------------------------------------
			threadTurn = decideTurn();
			//כוס אמק 
			createAndSendMessage(allSockets[0], GAME_STARTED,NULL, NULL, NULL);
			createAndSendMessage(allSockets[1], GAME_STARTED,NULL, NULL, NULL);
			//
			createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL);
			createAndSendMessage(allSockets[1], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL);
			// --------------- need to release mutex------------------------------------
			if(threadTurn == index)
			{
				createAndSendMessage(allSockets[threadTurn], SERVER_MOVE_REQUEST, NULL, NULL, NULL);
			}			
			break;
		case CLIENT_PLAYER_MOVE:
			// --------------- need to add mutex------------------------------------
			if (LEGAL_MOVE(arg))
			{
				game_state.Game_number++;
				createAndSendMessage(allSockets[0], GAME_VIEW, game_state.players[threadTurn], arg, "CONT");
				createAndSendMessage(allSockets[1], GAME_VIEW, game_state.players[threadTurn], arg, "CONT");
				
				createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
				createAndSendMessage(allSockets[1], TURN_SWITCH, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
				// --------------- need to release mutex------------------------------------
				continue;
			}
			game_state.game_ended = 1;
			createAndSendMessage(allSockets[0], GAME_VIEW, game_state.players[threadTurn], arg, "END");
			createAndSendMessage(allSockets[1], GAME_VIEW, game_state.players[threadTurn], arg, "END");
			
			createAndSendMessage(allSockets[0], GAME_ENDED, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
			createAndSendMessage(allSockets[1], GAME_ENDED, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
			
			ResetEvent(GameManagerHandle);
			
			createAndSendMessage(allSockets[0], SERVER_MAIN_MENU, NULL, NULL, NULL);
			createAndSendMessage(allSockets[1], SERVER_MAIN_MENU, NULL, NULL, NULL);
			// --------------- need to release mutex------------------------------------
			break;
		case CLIENT_DISCONNECT:
			ResetEvent(GameManagerHandle);
			socketCount--;
			break;
		default:
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	GameManagerHandle = CreateEvent(NULL, TRUE, FALSE, "Game_Manager_Handle");

	if (NULL == GameManagerHandle)
	{
		printf("Couldn't create event\n");
		return 1;
	}

	WSADATA wsa_data;
	int result;
	result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0) {
		printf("WSAStartup failed: %d\n", result);
		return 1;
	}
	// Create and init socket
	SOCKET server_s;
	if ((server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return 1;
	}
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int client_addrss_len, server_addrss_len = sizeof(server_addr);

	//Bind
	if (bind(server_s, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	// Setup timeval variable
	struct timeval timeout;
	timeout.tv_sec = 13;
	timeout.tv_usec = 0;

	struct fd_set fds;

	HANDLE Threads[2];
	DWORD threadIDs[10];
	int openedsuccessfuly[2];
	int type;

	
	int retval, received = 0;
	//char buffer[3] = { 0 };

	//wait for data to be sent by the channel
	int ListenRes = listen(server_s, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		//goto server_cleanup_2;
	}
	SOCKET client_s;
	while (1)
	{ // &client_addr
		if (socketCount < 2)
		{
			client_s = accept(server_s, NULL, NULL);
			if (client_s == INVALID_SOCKET)
			{
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
				//goto server_cleanup_3;
			}
			allSockets[socketCount] = client_s;
			socketCount++;


			//set descriptors
			FD_ZERO(&fds);
			FD_SET(client_s, &fds);

			retval = select(max(server_s, stdin) + 1, &fds, NULL, NULL, &timeout);
			if (retval < 0)
			{//error occured
				return 1;
				// check errno/WSAGetLastError(), call perror(), etc ...
			}

			if (retval > 0)
			{
				if (FD_ISSET(client_s, &fds))
				{//data was received from the channel
				//we will process it (decode) and write it to the output file

					//Receive data from the channel
					char* recieved = NULL;
					int Rec;
					//CLIENT_REQUEST 
					// 
					//type = getArgsFromMessage(received, ) --------------------------------------------------------------
				///// CHECK IF THERE ARE NO MORE THN 2 THREADS OR DENY
					if (openThread(&((Threads)[socketCount-1]), &threadExecute, &client_s, &(threadIDs[0])))
					{
						openedsuccessfuly[0] = 0; // if thread wasnt opened successfully
						printf("Error with thread %d\n", 0);
					}

					//TURN_SWITCHED 
					//Sleep(5000000);
					//break;
				}
			}
			if (retval == 0)
			{
				//TIMEOUT?
				continue;
			}
		}
	}
	// Deinitialize Winsock
	result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return 1;
	}
	if(!ReleaseSemaphore(GlobalSemaphore, 1, NULL))
	{
		printf("problem\n");
		return 1;
	}
	return 0;
}
