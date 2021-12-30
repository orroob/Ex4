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
				int buffer_len = recvfrom(client_s, buffer, 2, 0, (struct sockaddr*)&server_addr, &server_addrss_len); // ********* replace with client address
				if (buffer_len == 0)
				{
					return 1;
				}
				//printf(buffer);
				received += buffer_len;
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