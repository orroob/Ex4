#include "HardCodedData.h"
#include "SendReceiveHandling.h"
/// <summary>
/// Get message type and arranfe a buffer that contains the message needed to be sent
/// </summary>
/// <param name="messageType"> int from defined message types </param>
/// <param name="arg1"> name of client or his move </param>
/// <returns></returns>
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
		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "CLIENT_REQUEST:%s\n", arg1);
		break;

	case CLIENT_VERSUS:
		if ((buffSize = snprintf(NULL, 0, "CLIENT_VERSUS\n")) == 0) //snprintf returns num of characters
		{
			return NULL;
		}

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
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

		if ((buffer = malloc((buffSize + 1) * sizeof(char))) == NULL)
		{
			return NULL;
		}
		sprintf_s(buffer, buffSize + 1, "CLIENT_PLAYER_MOVE:%s\n", arg1);
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
/// <summary>
/// put in buffer the message and use a func that will senf it to the server
/// </summary>
/// <param name="messageType"> int from defined messages</param>
/// <param name="arg1"> name/move</param>
/// <param name="client_s">SOCKET</param>
/// <returns></returns>
int createAndSendMessage(int messageType, char* arg1, SOCKET client_s) {
	char* buffer;
	buffer = PrepareMessage(messageType, arg1);
	if (SendString(buffer, client_s) != TRNS_SUCCEEDED) {
		printf("Server disconnected. Exiting.\n");
		//client finishes run and releases +closes all
		closesocket(client_s);
		free(buffer);
		//result = WSACleanup();
		//if (result != 0) {
		//	printf("WSACleanup failed: %d\n", result);
		//	return 1;
		//}
		return 1;
	}
	writeMessageToLogFile(buffer, SENT);
	free(buffer);
	return 0;
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

			return SOCKET_ERROR;
			Last_Error_Value = WSAGetLastError();
			if (Last_Error_Value == TIME_OUT_ERROR) {
				//printf("Server disconnected. Exiting.\n");
				return TRNS_TIMEOUT;
			}

			//printf("send() failed, error %d\n", WSAGetLastError());
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
/// receive a chunk of the buffer
/// </summary>
/// <param name="OutputBuffer"></param>
/// <param name="BytesToReceive"></param>
/// <param name="sd"></param>
/// <returns></returns>
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
/// <summary>
/// receive the buffer from the server while calling another func to receive smaller chunks of strings.
/// </summary>
/// <param name="OutputStrPtr"></param>
/// <param name="sd"></param>
/// <returns></returns>
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
/// <summary>
/// Calls the func which receives the string and return if succeded or failed
/// </summary>
/// <param name="AcceptedStr"></param>
/// <param name="client_s"></param>
/// <returns></returns>
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
	//free(AcceptedStr);
	//return 0;
}