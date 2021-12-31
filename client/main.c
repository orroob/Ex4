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


#define CLIENT_REQUEST 0
#define CLIENT_VERSUS 1
#define CLIENT_PLAYER_MOVE 2
#define CLIENT_DISCONNECT 3

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
		if ((buffSize = snprintf(NULL, 0, "CLIENT_VERSUS:\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc(buffSize * sizeof(char)) == NULL))
		{
			return NULL;
		}
		sprintf_s(buffer, "CLIENT_VERSUS:\n", arg1);
		break;
	case CLIENT_PLAYER_MOVE:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_PLAYER_MOVE:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc(buffSize * sizeof(char)) == NULL))
		{
			return NULL;
		}
		sprintf_s(buffer, "CLIENT_PLAYER_MOVE:%s\n", arg1);
		break;
	case CLIENT_DISCONNECT:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_DISCONNECT:\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc(buffSize * sizeof(char)) == NULL))
		{
			return NULL;
		}
		sprintf_s(buffer, "CLIENT_DISCONNECT:\n",arg1);
		break;
	default:
		buffer = NULL;
	}
	return buffer;
}

int main(int argc, char* argv[])
{
	printf("%s\n%d\n%s\n", argv[1], atoi(argv[2]), argv[3]);
	// Initialize Winsock
	WSADATA wsa_data;
	int result;
	result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (result != 0) {
		printf("WSAStartup failed: %d\n", result);
		return 1;
	}

	// Create and init socket
	SOCKET client_s;
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

	char* buffer, input[10] = { 0 };

	// Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if (connect(client_s, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("Failed to connect to server on %s:%s.\nChoose what to do next:\n1. Try to reconnect\n2. Exit", argv[1], argv[2]);
		gets(input);
		WSACleanup(); 
		return 1;
	} // WSAGetLastError()

	printf("Connected to server on %s:%s.\n" ,argv[1], argv[2]);
	// Setup timeval variable and fd_set variable
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0.5;
	
	struct fd_set fds;
	
	
	//buffer = PrepareMessage(CLIENT_REQUEST, "or");
	//if (buffer == NULL)
	//{
	//	printf("oops");
	//	return 1;
	//}
	//Sleep(5000);
	//if (sendto(client_s, buffer, strlen(buffer) - 1, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	//{
	//	printf("sendto() failed with error code : %d", WSAGetLastError());
	//	return 1;
	//}
		 
	//CLIENT_REQUEST
	buffer =PrepareMessage(CLIENT_REQUEST, argv[3]);
	 Sleep(5000);
	if (sendto(client_s, buffer, strlen(buffer)-1, 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		return 1;
	}
	// LOOP:
	while(1)
	{
		//recv SERVER_MAIN_MENU

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
			if (sendto(client_s, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				return 1;
			}
		}
		//DISCONNECT_CLIENT
		if (!strcmp(input, "2"))
		{
			buffer = PrepareMessage(CLIENT_DISCONNECT, NULL);
			if (sendto(client_s, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				return 1;
			}
		}
	
		while (1)
		{
			//recv GAME_STARTED
			//recv TURN_SWITCH
			//recv SERVER_MOVE_REQUEST / TURN_SWITCH
			gets(input);
			buffer = PrepareMessage(CLIENT_PLAYER_MOVE, input);
			if (sendto(client_s, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				return 1;
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
	return 0;
}