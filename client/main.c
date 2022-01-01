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


SOCKET client_s;
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
int getArgsFromMessage(char* message, char arg1)
{
	char mType[20] = { 0 };
	char *temp = message;
	while (temp != NULL && *temp != ':')
	{
		temp++;
	}
	if (temp == NULL)
		return -1;

	*temp = '\0';
	strcpy(mType, message);

	if (!strcmp(mType, "GAME_ENDED"))
	{

		return GAME_ENDED;
	}
	if (!strcmp(mType, "TURN_SWITCH"))
		return TURN_SWITCH;
	if (!strcmp(mType, "GAME_VIEW"))
		return GAME_VIEW;
}
int transMessageToInt(char* message)
{
	if (!strcmp(message, "SERVER_APPROVED"))
		return SERVER_APPROVED;
	if (!strcmp(message, "SERVER_DENIED"))
		return SERVER_DENIED;
	if (!strcmp(message, "SERVER_MAIN_MENU"))
		return SERVER_MAIN_MENU;
	if (!strcmp(message, "GAME_STARTED"))
		return GAME_STARTED;
	if (!strcmp(message, "SERVER_MOVE_REQUEST"))
		return SERVER_MOVE_REQUEST;
	if (!strcmp(message, "SERVER_OPPONENT_QUIT"))
		return SERVER_OPPONENT_QUIT;
	if (!strcmp(message, "CLIENT_PLAYER_MOVE"))
		return CLIENT_PLAYER_MOVE;
	//with arguments:
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

		if ((buffer = malloc(buffSize * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, "CLIENT_DISCONNECT\n",arg1);
		break;
	default:
		buffer = NULL;
	}
	return buffer;
}
int Play_Game() {
	int Rec, type;
	char* recieved=NULL;
	Rec = RecvDataThread(&recieved);
	if (Rec == TRNS_SUCCEEDED) {

		type = transMessageToInt(recieved);
		switch (type)
		{
		case GAME_STARTED:
			printf("Game is on!");
			break;
		case TURN_SWITCH:
		//	if (name == ourName)
		//	{
		//		receive SERVER_MOVE_REQUEST;
		//		send CLIENT_PLAYER_MOVE;
		//	}
			break;
		case GAME_VIEW:
			//write to log
			break;
		case GAME_ENDED:
			break;
		}
		
	}
}


int main(int argc, char* argv[])
{
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

	// Connect to a server.

	//Create a sockaddr_in object client_addr and set values.
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));			// port address
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);		// IP address
	int client_addr_len = sizeof(server_addr);

	char *buffer, *recieved =NULL , input[10] = { 0 };

	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	//loop
	while (1) {
		if (connect(client_s, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			printf("Failed to connect to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit", argv[1], argv[2]);
			gets(input);
			WSACleanup();
			return 1;
		}
		printf("Connected to server on %s:%s.\n", argv[1], argv[2]);
		// Setup timeval variable and fd_set variable
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0.5;

		struct fd_set fds;
		//CLIENT_REQUEST
		buffer = PrepareMessage(CLIENT_REQUEST, argv[3]);
		Sleep(1000);
		SendString(buffer, client_s);
		int Rec = RecvDataThread(&recieved);
		if (Rec == TRNS_SUCCEEDED) {
			if (!strcmp(recieved, "SERVER_DENIED\n")) {
				printf("Server on %s:%s denied the connection request.\nChoose what to do next:\n1. Try to reconnect\n2. Exit\n", argv[1], argv[2]);
				// if input is 1, Try to connect again
				if (gets(input)) {
					continue;
				}
				else {
					//EXIT
				}
			}
			else if (!strcmp(recieved, "SERVER_APPROVED\n"))
			{
				printf("Choose what to do next:\n1. Play against another client\n2. Quit\n");
				gets(input);

				//Take in considiration Quiting
			}
			break;
		}
	}
	
	// LOOP:
	while (1)
	{
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
		}
		//recv GAME_STARTED
		free(recieved);
		recieved = NULL;
		int Rec = RecvDataThread(&recieved);
		if (Rec == TRNS_SUCCEEDED) {
			if (!strcmp(recieved, "GAME_STARTED\n")) {
				printf("Game is on!\n");
			}
			else {
				printf("ERROR\n");
			}
		}
		while (1)
		{
		//recv TURN_SWITCH
			free(recieved);
			recieved = NULL;
			Rec = RecvDataThread(&recieved);
			if (Rec == TRNS_SUCCEEDED) {

				if (!strcmp(recieved, "TURN_SWITCH:Philip\n")) { ////TEMPORARY
					free(recieved);
					recieved = NULL;
					//SERVER_MOVE_REQUEST
					Rec = RecvDataThread(&recieved);
				}
				else if (!strcmp(recieved, "TURN_SWITCH:%s\n", "Other Player Name")) {
					//GAME_VIEW wait 
					free(recieved);
					recieved = NULL;
					Rec = RecvDataThread(&recieved);
					if (Rec == TRNS_SUCCEEDED) {
					//	if (!strcmp(recieved, "GAME_VIEW\n")) {/////////////////////// ARGUMENTS
					//	}
					}
					// wait for another turn switch
				}
					if (Rec == TRNS_SUCCEEDED) {
						if (!strcmp(recieved, "SERVER_MOVE_REQUEST\n")) {
							printf("Enter the next number or boom:\n");
							gets(input);
							buffer = PrepareMessage(CLIENT_PLAYER_MOVE, input);
							if (SendString(buffer, client_s) != TRNS_SUCCEEDED) {
								printf("sendto() failed with error code : %d", WSAGetLastError());
								closesocket(client_s);
								free(buffer);
								return 1;
											}

									}
							}
	
				free(recieved);
				recieved = NULL;
				// RECV GAME_VIEW
				Rec = RecvDataThread(&recieved);
				if (Rec == TRNS_SUCCEEDED) {
					if (!strcmp(recieved, "GAME_VIEW:%s;%s;%s\n","Other player", "Move", "CONT\DONE")) {
					if(1){
						//if  not done
						if (1) {
							continue;
						}
						else{

							//finish game 
							return 1;//
						}
					}

				}
			}

		}
	}
	
	

	//VERSUS_CLIENT / DISCONNECT_CLIENT
	//recv STARTED_GAME
	// loop:
	//recv SWITCH_TURN
	//**wait until** recv REQUEST_MOVE_SERVER
	//CLIENT_PLAYER_MOVE
	//recv GAME_VIEW
	//end loop
	// recv ENDED_GAME
	//end LOOP


	printf("Success");
	// Deinitialize Winsock
	result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return 1;
	}
	free(buffer);
	//free(recieved);
	return 0;
}