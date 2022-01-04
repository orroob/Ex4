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

typedef struct Game_STATUS
{
	char* clientName;
	char* lastMove;
	int gameEnded;
}GAME;

GAME gameStruct;
SOCKET client_s;
char clientName[20];




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

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

int SendString(const char* Str, SOCKET sd)
{
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
		temp1 = strchr(temp + 1, ';');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);
		return GAME_ENDED;
	}
	if (!strcmp(mType, "TURN_SWITCH")) {
		temp1 = strchr(temp + 1, '\n');
		if (temp1 != NULL)
			*temp1 = '\0';
		strcpy(*arg1, temp + 1);                                            //arg1 doesnt go through!!!!!!!
		//arg1 = '\0';               ///////////////////////////////////
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

int RecvDataThread(char** AcceptedStr)
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
			printf("Server closed connection. Bye!\n");
			return TRNS_DISCONNECTED;
			//return 0x555;
		}
		else if (RecvRes == TRNS_TIMEOUT) {
			printf("Server timedout. Bye!\n");
			return TRNS_TIMEOUT;
			//return 0x555;
		}
		return TRNS_SUCCEEDED;
		//else if (!strcmp (AcceptedStr,"SERVER_DENIED")) {
		//	printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", arg1, arg2);
		//	return TRNS_SUCCEEDED;
		//	//return 0x555;
		//}
		//else if (!strcmp(AcceptedStr,"SERVER_APPROVED"))
		//{
		//	printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
		//	return TRNS_SUCCEEDED;
		//}

		//free(AcceptedStr);
		//return 0;
}

char* PrepareMessage(int messageType, char* arg1)
{
	
	char* buffer;
	int buffSize;
	switch (messageType)
	{
	case CLIENT_REQUEST:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_REQUEST:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize+1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		//printf("prepare m started\n");
		sprintf_s(buffer, buffSize+1, "CLIENT_REQUEST:%s\n", arg1);
		break;
	
	case CLIENT_VERSUS:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_VERSUS\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize+1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "CLIENT_VERSUS\n");
		break;
	case CLIENT_PLAYER_MOVE:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_PLAYER_MOVE:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize +1)* sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize+1, "CLIENT_PLAYER_MOVE:%s\n", arg1);
		break;
	case CLIENT_DISCONNECT:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_DISCONNECT\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "CLIENT_DISCONNECT\n");
		break;
	default:
		buffer = NULL;
	}
	return buffer;
}

int PlayGame()
{
	char* received = NULL, * buffer = NULL;
	char input[10] = { 0 };
	int Rec, type;
	DWORD timeout = 150000;
	char* name, * otherPlayerMove, * gameStatus;
	name = malloc(20 * sizeof(char));
	otherPlayerMove = malloc(20 * sizeof(char));
	gameStatus = malloc(20 * sizeof(char));
	while (!gameStruct.gameEnded)
	{
			//TRUN_SWITCH
		if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
			printf("setsockopt failed\n");
		Rec = RecvDataThread(&received);     ///////////STUCK ON RECV FOR  palyer number 2
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
					Rec = RecvDataThread(&received);
					if (Rec == TRNS_SUCCEEDED)
					{
						type = transMessageToInt(received, NULL, NULL , NULL);       
						if (type == SERVER_MOVE_REQUEST)
						{
							printf("Enter the next number or boom:\n");
							gets(input);                                            
							buffer = PrepareMessage(CLIENT_PLAYER_MOVE, input);
							if (SendString(buffer, client_s) != TRNS_SUCCEEDED)
							{
								printf("send failed with error code : %d", WSAGetLastError());
								//closesocket(client_s);
								free(name);
								free(otherPlayerMove);
								free(gameStatus);
								free(buffer);
								return 1;
							}
						}
					}
				}
				else {//other player's turn
					printf("%s's turn!\n", name);
				}
				break;
			case GAME_VIEW:
				printf("%s move was %s\n%s\n", name, otherPlayerMove, gameStatus); //check if both '\n\' are needed ------------------------
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
	char ch = '\n';
	//strncat(gameStruct.clientName, &ch, 1); /////////               append \n to name for strcmp with name 
	// Initialize Winsock
	WSADATA wsa_data;
	int result;
	result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0) {
		printf("WSAStartup failed: %d\n", result);
		return 1;
	}

	// Create and init socket

	if ((client_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return 1;
	}



	//Create a sockaddr_in object client_addr and set values.
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));			// port address
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);		// IP address
	int client_addr_len = sizeof(server_addr);

	char* buffer, * recieved = NULL, input[10] = { 0 };
	//struct timeval timeout;
	int Rec, type;

	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	//loop
	while (1) {		//try to connect to server
		if (connect(client_s, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
		{
			printf("Failed to connect to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", argv[1], argv[2]);
		get_input:
			gets(input);
			if (!strcmp(input, "1"))
				continue;
			if (!strcmp(input, "2"))
				break;
			else
			{
				printf("Error: illegal command\n");
				goto get_input;
			}
		}
		printf("Connected to server on %s:%s.\n", argv[1], argv[2]);
		// Setup timeval variable and fd_set variable
		//timeout.tv_sec = 15;
		//timeout.tv_usec = 0;

		struct fd_set fds;
		//CLIENT_REQUEST
		buffer = PrepareMessage(CLIENT_REQUEST, argv[3]);
		Sleep(1000);
		SendString(buffer, client_s);

		free(recieved);
		recieved = NULL; //// ------------------------------------ make sure all "received" are free
		Rec = RecvDataThread(&recieved);
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
				if (!strcmp(input, "2"))
					break;
				else
				{
					printf("Error: illegal command\n");
					goto get_input2;
				}
			}
			else if (!strcmp(recieved, "SERVER_APPROVED\n"))
			{
				//printf("Choose what to do next:\n1. Play against another client\n2. Quit\n"); //////////////////////
				//gets(input);

				//Take in considiration Quiting
			}
			break;
		}
	}
	//we are now connected to the server
	//DWORD timeout1 = 15000000;
	// MAIN MENU
	free(recieved);
	recieved = NULL;
	//RECV MAIN MENU
get_main_menu:
	//if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout1, sizeof timeout1) < 0)
	//	printf("setsockopt failed\n");
	Rec = RecvDataThread(&recieved);
	if (Rec == TRNS_SUCCEEDED)
	{
		if (transMessageToInt(recieved, NULL, NULL, NULL) == SERVER_MAIN_MENU)
		{
			printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
			gets(input);
			while (strcmp(input, "2") && strcmp(input, "1"))
			{
				printf("Error: Illegal command\n");
				printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
				gets(input);
			}
			//CLIENT_VERSUS
			if (!strcmp(input, "1"))
			{
				buffer = PrepareMessage(CLIENT_VERSUS, NULL);
				if (SendString(buffer, client_s) != TRNS_SUCCEEDED) {
					printf("Failed to send Server approved");
				}
			}
			//DISCONNECT_CLIENT 
			if (!strcmp(input, "2"))
			{
				buffer = PrepareMessage(CLIENT_DISCONNECT, NULL);
				if (SendString(buffer, client_s) != TRNS_SUCCEEDED) {
					printf("sendto() failed with error code : %d", WSAGetLastError());
					closesocket(client_s);
					free(buffer);
					return 1;
				}
				Sleep(1000);
				closesocket(client_s);
				result = WSACleanup();
				if (result != 0) {
					printf("WSACleanup failed: %d\n", result);
					return 1;
				}
				return 1;
			}
		}
	}

	// CLIENT_VERSUS was chosen
	free(recieved);
	recieved = NULL;
	DWORD timeout = 300000000;
	if (setsockopt(client_s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
		printf("setsockopt failed\n");
	Rec = RecvDataThread(&recieved);     
	if (Rec == TRNS_SUCCEEDED)
	{
		if ((type=transMessageToInt(recieved, NULL, NULL, NULL)) == GAME_STARTED)              ///////////RECV TURN SWITCH rather than GAME_STARTED Problem with Server???
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
	}

	result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return 1;
	}
	//free(buffer);
	//free(recieved);
	return 0;
}
