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

//receive options
#define TRNS_FAILED 0
#define TRNS_DISCONNECTED 1
#define TRNS_SUCCEEDED 2
#define TRNS_TIMEOUT 3
#define TIME_OUT_ERROR 0
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
char* PrepareMessage(int messageType, char* arg1, char* arg2, char* arg3)
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
		sprintf_s(buffer, buffSize + 1, "SERVER_APPROVED\n", arg1);
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
		sprintf_s(buffer, buffSize + 1, "SERVER_DENIED\n", arg1);
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
		sprintf_s(buffer, buffSize + 1, "SERVER_MAIN_MENU\n",arg1);
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
		sprintf_s(buffer, buffSize + 1, "GAME_STARTED\n", arg1);
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
		if ((buffSize = snprintf(NULL, 0, "SERVER_MOVE_REQUEST\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_MOVE_REQUEST\n", arg1);
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
		sprintf_s(buffer, buffSize + 1, "GAME_ENDED:$s\n", arg1);
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
		sprintf_s(buffer, buffSize + 1, "SERVER_NO_OPPONENTS\n", arg1);
		break;

	case GAME_VIEW:
		if ((buffSize = snprintf(NULL, 0, "GAME_VIEW:%s;%s;%s\n")) == 0,arg1, arg2, arg3) //snprintf returns num of characters
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
		if ((buffSize = snprintf(NULL, 0, "SERVER_OPPONENT_QUIT:%s\n")) == 0, arg1) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_OPPONENT_QUIT\n", arg1);
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




DWORD WINAPI threadExecute(SOCKET *client_s) {
	//char buffer[100] = { 0 };
	//strcpy(buffer, PrepareMessage(0, "Philip" , NULL, NULL));
	char* send = "SERVER_APPROVED";
	SendString(send, *client_s);
	return 0;
}

int main(int argc, char* argv[])
{

	//char a[100] = { 0 };
	//strcpy(a, PrepareMessage(4, "Philip", NULL, NULL));
	// Initialize Winsock

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
	struct fd_set fds;

	timeout.tv_sec = 13;
	timeout.tv_usec = 0;

	int retval, received=0;
	char buffer[3] = { 0 };


	//wait for data to be sent by the channel

	int ListenRes = listen(server_s, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		//goto server_cleanup_2;
	}

	SOCKET client_s = accept(server_s, NULL, NULL);
	if (client_s == INVALID_SOCKET)
	{
		printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
		//goto server_cleanup_3;
	}
	HANDLE Threads[2];
	DWORD threadIDs[10];
	int openedsuccessfuly[2];
	while (1)
	{ // &client_addr
	
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
				char* recieved=NULL;
				int Rec;
				Rec = RecvDataThread(&recieved, client_s);
				//int buffer_len = recvfrom(client_s, buffer, 2, 0, (struct sockaddr*)&server_addr, &server_addrss_len); // ********* replace with client address
	///// CHECK IF THERE ARE NO MORE THN 2 THREADS OR DENY
				if (openThread(&((Threads)[0]), &threadExecute, &client_s , &(threadIDs[0])))
				{
					openedsuccessfuly[0] = 0; // if thread wasnt opened successfully
					printf("Error with thread %d\n", 0);
				}
				//if (buffer_len == 0)
				//{
				//	return 1;
				//}
				//printf(buffer);
				//received += buffer_len;
				Sleep(50000);
				break;
			}
		}
		if (retval == 0)
		{
			continue;
		}
	}
	
	// Deinitialize Winsock
	result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		return 1;
	}
	return 0;
}