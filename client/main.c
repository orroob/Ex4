#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#pragma comment(lib, "Ws2_32.lib")
#include "HardCodedData.h"
#include "SendReceiveHandling.h"

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

//globals
GAME gameStruct;
SOCKET client_s;
char clientName[20];

int cleanupAll(char* recieved) {
	int result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return 1;
	}
	closesocket(client_s);
	free(recieved);
	return 0;
}

int getArgsFromMessage(char* message, char **arg1, char** arg2, char** arg3)
{
	char mType[20] = { 0 };
	int i = 0;
	char* temp = message, * temp1;
	while (temp != NULL && *temp != ':')          /////////DOESNT WORK FOR MOVE_REQUEST INFINITE LOOP
	{
		mType[i] = *temp;
		temp++;
		i++;
	}
	if (temp == NULL)
		return -1;
	mType[i] = '\0';   ///FAILS
	*temp = '\0';
	//strcpy(mType, message);

	if (!strcmp(mType, "GAME_ENDED"))
	{
		temp1 = strchr(temp + 1, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);
		return GAME_ENDED;
	}
	if (!strcmp(mType, "TURN_SWITCH")) {
		temp1 = strchr(temp + 1, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);                                           
		return TURN_SWITCH;
	}
	if (!strcmp(mType, "GAME_VIEW"))
	{
		temp1 = strchr(temp + 1, ';');
		*temp1 = '\0';
		strcpy(*arg1, temp + 1);
		temp = temp1 + 1;
		temp1 = strchr(temp, ';');
		*temp1 = '\0';
		strcpy(*arg2, temp);
		temp = temp1 + 1;
		temp1 = strchr(temp, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg3, temp);
		return GAME_VIEW;
	}
	return -1;
}

int transMessageToInt(char* message, char** arg1, char** arg2, char** arg3)
{
	if (!strcmp(message, "SERVER_APPROVED\n"))
		return SERVER_APPROVED;
	if (!strcmp(message, "SERVER_DENIED\n"))
		return SERVER_DENIED;
	if (!strcmp(message, "SERVER_MAIN_MENU\n"))
		return SERVER_MAIN_MENU;
	if (!strcmp(message, "GAME_STARTED\n"))
		return GAME_STARTED;
	if (!strcmp(message, "SERVER_MOVE_REQUEST\n"))
		return SERVER_MOVE_REQUEST;
	if (!strcmp(message, "SERVER_OPPONENT_QUIT\n"))
		return SERVER_OPPONENT_QUIT;
	if (!strcmp(message, "SERVER_NO_OPPONENTS\n"))
		return SERVER_NO_OPPONENTS;
	//with arguments:
	return getArgsFromMessage(message, arg1, arg2, arg3);
}

char* readinput() {    /// NEED TO FREE INPUT

	char* input = NULL;
	char tempbuf[CHUNK];
	size_t inputlen = 0, templen = 0;
	do {
		fgets(tempbuf, CHUNK, stdin);
		templen = strlen(tempbuf);
		input = realloc(input, inputlen + templen + 1);
		strcpy(input + inputlen, tempbuf);
		inputlen += templen;
	} while (templen == CHUNK - 1 && tempbuf[CHUNK - 2] != '\n');
	
	char* temp = input;
	while ((temp + 1) != NULL && *temp != '\n')
	{
		temp++;
	}
	if ((temp + 1) != NULL)
		*temp = '\0';
	return input;
}

int PlayGame()
{
	char* received = NULL, * buffer = NULL;
	char *input;
	
	int Rec, type;
	DWORD timeout = 1500000;
	char* name, * otherPlayerMove, * gameStatus;
	name = malloc(20 * sizeof(char));
	otherPlayerMove = malloc(20 * sizeof(char));
	gameStatus = malloc(20 * sizeof(char));
	while (!gameStruct.gameEnded)
	{
			//TRUN_SWITCH
		if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
			printf("setsockopt failed\n");
		Rec = RecvDataThread(&received, client_s);     ///////////STUCK ON RECV FOR  palyer number 2
		if (Rec == TRNS_SUCCEEDED)
		{
			type = transMessageToInt(received, &name, &otherPlayerMove, &gameStatus);
			switch (type)
			{
			case TURN_SWITCH:
				if (!strcmp(name, gameStruct.clientName))    
				{//our turn 
					printf("Your turn!\n");
					free(received);
					received = NULL;
					//SERVER_MOVE_REQUEST
					if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
						printf("setsockopt failed\n");
					Rec = RecvDataThread(&received, client_s);
					if (Rec == TRNS_SUCCEEDED)
					{
						type = transMessageToInt(received, NULL, NULL , NULL);       
						if (type == SERVER_MOVE_REQUEST)
						{
							printf("Enter the next number or boom:\n");
							input =readinput();
							//gets(input);   
							if (setsockopt(client_s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof timeout) < 0)
								printf("setsockopt failed\n");
							if (createAndSendMessage(CLIENT_PLAYER_MOVE, input, client_s)) {
								free(name);
								free(otherPlayerMove);
								free(gameStatus);
								free(input);
								return 1;
							}

							//buffer = PrepareMessage(CLIENT_PLAYER_MOVE, input);
							//Sleep(500);
							//if (SendString(buffer, client_s) != TRNS_SUCCEEDED)
							//{
							//	printf("send failed with error code : %d", WSAGetLastError());
							//	//closesocket(client_s);
							//	free(name);
							//	free(otherPlayerMove);
							//	free(gameStatus);
							//	free(buffer);
							//	return 1;
							//}
						}
					}
				}
				else {//other player's turn
					printf("%s's turn!\n", name);
				}
				break;
			case GAME_VIEW:
				printf("%s move was %s %s\n", name, otherPlayerMove, gameStatus); //check if both '\n\' are needed ------------------------
				break;
			case GAME_ENDED:
				printf("%s won!\n", name);
				gameStruct.gameEnded = 1;
				break;
			case SERVER_OPPONENT_QUIT:
				break;
			}
		}
		free(received);
		received = NULL;
	}
	free(name);
	free(otherPlayerMove);
	free(gameStatus);
	return 0;
}

int main(int argc, char* argv[])
{
	gameStruct.clientName = argv[3];
	// Initialize Winsock
	WSADATA wsa_data;
	int result;
	result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0) {
		printf("WSAStartup failed: %d\n", result);
		return 1;
	}
	if ((client_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) // Create and init socket
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return 1;
	}
	struct sockaddr_in server_addr; //Create a sockaddr_in object client_addr and set values.
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));			// port address
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);		// IP address
	int client_addr_len = sizeof(server_addr) , Rec, type;
	char* recieved = NULL, *input = NULL;
	//Call the connect function, passing the created socket and the sockaddr_in structure as parameters. Check for general errors.
	//loop
	DWORD timeout = 15000000;
	while (1) {		//try to connect to server
		if (connect(client_s, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
		{
		get_input:
			printf("Failed to connect to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", argv[1], argv[2]);
			input = readinput();
			if (!strcmp(input, "1"))
				continue;
			if (!strcmp(input, "2"))
				return cleanupAll(recieved);  //exit code
			else
			{
				printf("Error: illegal command\n");
				goto get_input;
			}
		}
		printf("Connected to server on %s:%s.\n", argv[1], argv[2]);
		//CLIENT_REQUEST
		if (createAndSendMessage(CLIENT_REQUEST, argv[3], client_s)) {
			return 1; // END 
		 }
		free(recieved);
		recieved = NULL; 
		if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0) {
		
			/// ///Do something? should you disconnect? or try again?
			
			printf("setsockopt failed\n");
		}
		Rec = RecvDataThread(&recieved, client_s);
		if (Rec == TRNS_SUCCEEDED)
		{
			if (!strcmp(recieved, "SERVER_DENIED\n"))
			{
			get_input2:
				printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", argv[1], argv[2]);
				// if input is 1, Try to connect again
				if (!strcmp(input, "1")) {
					continue;
				}
				if (!strcmp(input, "2")) {
					closesocket(client_s);
					result = WSACleanup();
					if (result != 0) {
						printf("WSACleanup failed: %d\n", result);
						return 1;
					}
					return 1;
				}
				else
				{
					printf("Error: illegal command\n");
					goto get_input2;
				}
			}
			//Seems we can skip this
		//else if (!strcmp(recieved, "SERVER_APPROVED\n"))
		//{
		//	//Take in considiration Quiting
		//}
			break;
		}
	}
	free(recieved);
	recieved = NULL;
	//we are now connected to the server RECV MAIN MENU
get_main_menu:                        
	if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0) {

		// Do Something

		printf("setsockopt failed\n");
	}
	Rec = RecvDataThread(&recieved, client_s);
	if (Rec == TRNS_SUCCEEDED)
	{
		if (transMessageToInt(recieved, NULL, NULL, NULL) == SERVER_MAIN_MENU)
		{
			printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
			input = readinput();
			while (strcmp(input, "2") && strcmp(input, "1"))
			{
				printf("Error: Illegal command\nChoose what to do next:\n1. Play against another client\n2. Quit\n");
				input = readinput();
			}
			if (!strcmp(input, "1"))//CLIENT_VERSUS
			{
				Sleep(3000);
				if (createAndSendMessage(CLIENT_VERSUS, NULL, client_s)) {
					return 1; // END 
				}
			}
			if (!strcmp(input, "2")) //DISCONNECT_CLIENT 
				if (createAndSendMessage(CLIENT_DISCONNECT, NULL, client_s)) {
					return 1; // END 
			}
		}
	}
	// CLIENT_VERSUS was chosen
	free(recieved);
	recieved = NULL;
	DWORD timeout2 = 30000; //Wait for 30 seconds for a chance to get an opponent
	if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout2, sizeof timeout2) < 0)
		printf("setsockopt failed\n");
	Rec = RecvDataThread(&recieved, client_s);
	if (Rec == TRNS_SUCCEEDED)
	{
		if ((type=transMessageToInt(recieved, NULL, NULL, NULL)) == GAME_STARTED)   
		{
			free(recieved);
			recieved = NULL;
			printf("Game is on!\n");
			PlayGame();
		}
		if (type == SERVER_NO_OPPONENTS)
		{
			free(recieved);
			recieved = NULL;
			goto get_main_menu;
		}
		if (type == SERVER_OPPONENT_QUIT)          /////////////NEW
		{
			printf("Opponent Quit.\n");
			free(recieved);
			recieved = NULL;
			goto get_main_menu;
		}
	}
	goto get_main_menu;	
	cleanupAll(recieved);
}
