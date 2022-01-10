#include "HardCodedData.h"
#include "SendReceiveHandling.h"
/// <summary>
/// receive a chunk of the buffer
/// </summary>
/// <param name="OutputBuffer"></param>
/// <param name="BytesToReceive"></param>
/// <param name="sd"></param>
/// <returns></returns>
int ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd)
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

/// <summary>
/// Get message type and arranfe a buffer that contains the message needed to be sent
/// </summary>
/// <param name="messageType"> int from defined message types </param>
/// <param name="arg1"> name of client or his move </param>
/// <returns></returns>
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
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_APPROVED\n");
		break;
	case SERVER_DENIED:
		if ((buffSize = snprintf(NULL, 0, "SERVER_DENIED\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_DENIED\n");
		break;
	case SERVER_MAIN_MENU:
		if ((buffSize = snprintf(NULL, 0, "SERVER_MAIN_MENU:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_MAIN_MENU\n");
		break;
	case GAME_STARTED:
		if ((buffSize = snprintf(NULL, 0, "GAME_STARTED\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "GAME_STARTED\n");
		break;
	case TURN_SWITCH:
		if ((buffSize = snprintf(NULL, 0, "TURN_SWITCH:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "TURN_SWITCH:%s\n", arg1);
		break;
	case SERVER_MOVE_REQUEST:
		if ((buffSize = snprintf(NULL, 0, "SERVER_MOVE_REQUEST\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_MOVE_REQUEST\n");
		break;
	case GAME_ENDED:
		if ((buffSize = snprintf(NULL, 0, "GAME_ENDED:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "GAME_ENDED:%s\n", arg1);
		break;
	case SERVER_NO_OPPONENTS:
		if ((buffSize = snprintf(NULL, 0, "SERVER_NO_OPPONENTS\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_NO_OPPONENTS\n");
		break;
	case GAME_VIEW:
		if ((buffSize = snprintf(NULL, 0, "GAME_VIEW:%s;%s;%s\n", arg1, arg2, arg3)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "GAME_VIEW:%s;%s;%s\n", arg1, arg2, arg3);
		break;
	case SERVER_OPPONENT_QUIT:
		if ((buffSize = snprintf(NULL, 0, "SERVER_OPPONENT_QUIT:%s\n", arg1)) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL) {
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "SERVER_OPPONENT_QUIT\n");
		break;
	default:
		buffer = NULL;
	}
	return buffer;
}

/// <summary>
/// func that sends a chunk of the buffer
/// </summary>
/// <param name="Buffer"></param>
/// <param name="BytesToSend"></param>
/// <param name="sd"></param>
/// <returns></returns>
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

/// <summary>
/// func that calls another func that will send smaller chunks of the buffer
/// </summary>
/// <param name="Str"></param>
/// <param name="sd"></param>
/// <returns></returns>
int SendString(const char* Str, SOCKET sd)
{
	//Sleep(5000);
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

/// <summary>
/// put in buffer the message and use a func that will senf it to the server
/// </summary>
/// <param name="messageType"> int from defined messages</param>
/// <param name="arg1"> name/move</param>
/// <param name="client_s">SOCKET</param>
/// <returns></returns>
int createAndSendMessage(SOCKET client_s, int messageType, char* arg1, char* arg2, char* arg3, int index)
{
	char* send;
	send = PrepareMessage(messageType, arg1, arg2, arg3);
	if (SendString(send, client_s) != TRNS_SUCCEEDED) {
		printf("Failed to send Server approved");
		return 1;
	}
	writeMessageToLogFile(send, SENT, index);
	free(send);
	return 0;
}
