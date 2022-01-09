#include "ProcessHandling.h"
#include "HardCodedData.h"
#include"SendReceiveHandling.h"

#define SENT 0
#define RECEIVED 1
/*
EX4
Philip Dolav 322656273
Or Roob      212199970
*/
//globals
Game game_state = { 0 };
SOCKET allSockets[MAX_SOCKETS];
HANDLE StartGameEvent;
HANDLE RestartGameEvent;
HANDLE GameStsteMutex;
HANDLE Threads[MAX_SOCKETS];
HANDLE LogFile;
int openedsuccessfuly[MAX_SOCKETS], socketCount = 0;
/// <summary>
/// WSAcleanup, closing socket and freeing 
/// </summary>
/// <returns> 1 f failed, 0 otherwise </returns>
int CleanUpAll()
{
	int ToReturn = 0;
	int result = WSACleanup();
	if (result != 0) {
		printf("WSACleanup failed: %d\n", result);
		ToReturn = 1;
	}
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		free(game_state.players[i]);
		if (allSockets[i] != NULL)
			closesocket(allSockets[i]);
	}
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (Threads[i] != NULL)
			closeFile(Threads[i]);
	}

	ToReturn |= (CloseHandle(StartGameEvent) || CloseHandle(GameStsteMutex));// || CloseHandle(StartGameMutex));
	return ToReturn;
}

/// <summary>
/// This function gets input from the user, in unknown length.
/// </summary>
/// <returns>The user's input.</returns>
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

/// <summary>
/// This function checks if a number should be turned to 'boon' according to 7-boom rools.
/// </summary>
/// <param name="number"> - an int</param>
/// <returns>Returns whether or not the number should be turned to 'boon'</returns>
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

/// <summary>
/// This function chaecks if the string equals to "boom"
/// </summary>
/// <param name="input">string</param>
/// <returns></returns>
int containsBOOM(char* input) {
	char BOOM[5] = "boom";
	if (!strcmp(BOOM, input)) {
		return 1; // if boom was the input 
	}
	return 0;
}

/// <summary>
/// This function decides whether or not a move is legal.
/// </summary>
/// <param name="next_move"> - the next move.</param>
/// <returns></returns>
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
	printf("FORGOT ssssssss\n");
	return 0;
}

/// <summary>
/// This function places the argument received from the client in arg1.
/// </summary>
/// <param name="message"> - the message received from the client.</param>
/// <param name="arg1"> - a pointer to char* object, where the argument will be placed.</param>
/// <returns></returns>
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
	return -1;
}

/// <summary>
/// This function get a message, returns its type and places its argument in arg1.
/// </summary>
/// <param name="message"></param>
/// <param name="arg1"></param>
/// <returns></returns>
int transMessageToInt(char* message, char** arg1)
{
	if (!strcmp(message, "CLIENT_VERSUS\n"))
		return CLIENT_VERSUS;
	if (!strcmp(message, "CLIENT_DISCONNECT\n"))
		return CLIENT_DISCONNECT;
	//with arguments:
	return getArgsFromMessage(message, arg1);
}

/// <summary>
/// This function is ran by a thread in the background and signals - RestartGameEvent when a game is over.
/// </summary>
/// <returns></returns>
DWORD WINAPI ResatartGame()
{
	while (1)
	{
		if (game_state.game_ended)
		{
			Sleep(2000);
			SetEvent(RestartGameEvent);
		}
	}
}

/// <summary>
/// This function is ran in a seperate thread for every client and manages the comunication with it.
/// </summary>
/// <param name="client_s"> - a pointer to a socket received from accept function.</param>
/// <returns></returns>
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
	
	printf("Thread_opened\n");
	SOCKET s = *client_s;
	int Rec = 0;
	while(!Rec)
	{
		Rec = PlayGame(s, socketCount - 1);
		WaitForSingleObject(RestartGameEvent, (DWORD)1000000);
		game_state.game_ended = 0;
		game_state.players_num--;
		game_state.Game_number = 0;
		//createAndSendMessage(s, SERVER_MAIN_MENU, NULL, NULL, NULL);     ///////breaks sometime??????
	}
	return 0;
}

/// <summary>
/// this function decides which thread's turn it is.
/// </summary>
/// <returns></returns>
int decideTurn()
{
	return game_state.Game_number % 2;	
}

/// <summary>
/// This function manages the comunication with the client when receiving CLIENT_REQUEST.
/// </summary>
/// <param name="client_s"></param>
/// <param name="index"></param>
/// <param name="arg"></param>
/// <returns></returns>
int Handle_CLIENT_REQUEST(SOCKET client_s, int index, char *arg)
{
	if (game_state.players_num >= 2)
	{
		//server denied
		createAndSendMessage(client_s, SERVER_DENIED, NULL, NULL, NULL);
		Sleep(1000);
		return 0;
	}
	//server accepted
	createAndSendMessage(client_s, SERVER_APPROVED, NULL, NULL, NULL);

	//add player's name to game struct
	WaitForSingleObject(GameStsteMutex, INFINITE);
	game_state.players[index] = malloc(20 * sizeof(char));
	strcpy(game_state.players[index], arg);
	ReleaseMutex(GameStsteMutex);

	//send MAIN MENU
	createAndSendMessage(client_s, SERVER_MAIN_MENU, NULL, NULL, NULL);     ///////breaks sometime??????
	return 0;
}

/// <summary>
/// This function manages the comunication with the client when receiving CLIENT_VERSUS.
/// </summary>
/// <param name="client_s"></param>
/// <param name="index"></param>
/// <param name="arg"></param>
/// <returns></returns>
int Handle_CLIENT_VERSUS(SOCKET client_s, int index)
{
	int isFirstPlayer = 0, canStartGame = 0, threadTurn;
	//inc players num 
	game_state.players_num++;
	//wait for other player to join (15 sec)
	if (game_state.players_num < 2) { // if this is the first player- wait forever for the second one
		//createAndSendMessage(allSockets[0], GAME_STARTED, NULL, NULL, NULL);
		//createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[0], NULL, NULL);
		//createAndSendMessage(allSockets[0], SERVER_MOVE_REQUEST, NULL, NULL, NULL);
		//return 0; ///////////////////////////////////////////////////////////////////////////// checkkkkk
		isFirstPlayer = 1;
			WaitForSingleObject(StartGameEvent, 150000);
	}
	if (game_state.players_num >= 2 && !isFirstPlayer)
	{
		SetEvent(StartGameEvent);
		return 0;
	}
	canStartGame = (game_state.players_num >= 2);
	if (!canStartGame)
	{
		game_state.game_started = 0;
		game_state.players_num--;
		createAndSendMessage(client_s, SERVER_NO_OPPONENTS, NULL, NULL, NULL);
		createAndSendMessage(client_s, SERVER_MAIN_MENU, NULL, NULL, NULL);
		return 0;
	}
	// --------------- need to add mutex------------------------------------
	printf("Got Here\n");
	WaitForSingleObject(GameStsteMutex, INFINITE);
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	threadTurn = decideTurn();
	//
	//Sleep(0);
	
	//if ((threadTurn = decideTurn()) != index) {
	//	return 0;
	//}
	game_state.game_started = 1;
	if (createAndSendMessage(allSockets[0], GAME_STARTED, NULL, NULL, NULL))
		printf("Error\n");
	if (createAndSendMessage(allSockets[1], GAME_STARTED, NULL, NULL, NULL))
		printf("Error\n");

	printf("GAME_Started from %d index\n", index);

	if (createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL))
		printf("Error\n");
	if (createAndSendMessage(allSockets[1], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL))
		printf("Error\n");

	// --------------- need to release mutex------------------------------------
	//if (threadTurn == index)
	//{
	createAndSendMessage(allSockets[threadTurn], SERVER_MOVE_REQUEST, NULL, NULL, NULL);
	//}
	ReleaseMutex(GameStsteMutex);
	return 0;
}

/// <summary>
/// This function manages the comunication with the client when receiving CLIENT_PLAYER_MOVE.
/// </summary>
/// <param name="client_s"> SOCKET </param>
/// <param name="index"> thread number</param>
/// <param name="arg"> player move </param>
/// <returns></returns>
int HANDLE_CLIENT_PLAYER_MOVE(char* arg)
{
	int threadTurn = decideTurn();
	if (LEGAL_MOVE(arg))
	{
		game_state.Game_number++;
		createAndSendMessage(allSockets[0], GAME_VIEW, game_state.players[threadTurn], arg, "CONT");
		createAndSendMessage(allSockets[1], GAME_VIEW, game_state.players[threadTurn], arg, "CONT");
		
		createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
		createAndSendMessage(allSockets[1], TURN_SWITCH, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
		
		createAndSendMessage(allSockets[(threadTurn + 1) % 2], SERVER_MOVE_REQUEST, NULL, NULL, NULL); // NEW TRY
	//	createAndSendMessage(allSockets[0], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL); //check delete
	//	createAndSendMessage(allSockets[1], TURN_SWITCH, game_state.players[threadTurn], NULL, NULL); //check delete 
	//	createAndSendMessage(allSockets[threadTurn], SERVER_MOVE_REQUEST, NULL, NULL, NULL); 
		// --------------- need to release mutex------------------------------------
		return 0;;
	}
	createAndSendMessage(allSockets[0], GAME_VIEW, game_state.players[threadTurn], arg, "END");
	createAndSendMessage(allSockets[1], GAME_VIEW, game_state.players[threadTurn], arg, "END");

	createAndSendMessage(allSockets[0], GAME_ENDED, game_state.players[(threadTurn + 1) % 2], NULL, NULL);
	createAndSendMessage(allSockets[1], GAME_ENDED, game_state.players[(threadTurn + 1) % 2], NULL, NULL);

	//ResetEvent(GameManagerHandle);

	createAndSendMessage(allSockets[0], SERVER_MAIN_MENU, NULL, NULL, NULL);
	createAndSendMessage(allSockets[1], SERVER_MAIN_MENU, NULL, NULL, NULL);

	game_state.game_ended = 1;
	game_state.game_started = 0;
	return 0;
}

/// <summary>
/// This function manages the comunication with the client when receiving CLIENT_DISCONNECT.
/// </summary>
/// <param name="client_s"> SOCKET </param>
/// <param name="index"> Thread Number</param>
/// <param name="arg"> input </param>
/// <returns></returns>
int HANDLE_CLIENT_DISCONNECT(int index, char* arg)
{
	//send other player MAIN_MENU
	createAndSendMessage(allSockets[(index + 1) % 2], SERVER_MAIN_MENU, NULL, NULL, NULL);
	game_state.players_num--;
	ResetEvent(StartGameEvent);
	socketCount--;
	free(arg);
	return 0;
}

/// <summary>
/// This function manages the game in front of the clients.
/// </summary>
/// <param name="s"> SOCKET </param>
/// <param name="index"> input </param>
/// <returns></returns>
int PlayGame(SOCKET s, int index)
{
	char* received = NULL, * arg;
	int Rec, type, threadTurn = 0, canStartGame, isFirstPlayer = 0;
	if ((arg = malloc(20 * sizeof(char))) == NULL)
		return 1;
	DWORD timeout = 150000;

	while (!game_state.game_ended)
	{
		if (game_state.game_started)
		{
			// reset the restart-game Event
			ResetEvent(RestartGameEvent);

			if (decideTurn() == index)
			{
				if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
					printf("setsockopt failed\n");
				Rec = RecvDataThread(&received, s);
				if (Rec == 0)
					continue;
				//writeMessageToLogFile(received, RECEIVED);
			}
		}
		else
		{
			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
				printf("setsockopt failed\n");
			Rec = RecvDataThread(&received,s);
			if (Rec == 0)
				continue;
			//writeMessageToLogFile(received, RECEIVED);
		}
		if (Rec != TRNS_SUCCEEDED) {
			continue;
		}
		type = transMessageToInt(received, &arg);
		free(received);
		received = NULL;

		switch (type)
		{
		case CLIENT_REQUEST:
			//---------------add mutex before approaching game_state -------------------------------
			Handle_CLIENT_REQUEST(s, index, arg);
			break;
		case CLIENT_VERSUS:
			//wait
			Handle_CLIENT_VERSUS(s, index);
			//relaese
			break;
		case CLIENT_PLAYER_MOVE:
			// --------------- need to add mutex------------------------------------
			WaitForSingleObject(GameStsteMutex, INFINITE);
			HANDLE_CLIENT_PLAYER_MOVE(arg);
			ReleaseMutex(GameStsteMutex);
			// --------------- need to release mutex------------------------------------
			break;
		case CLIENT_DISCONNECT:
			HANDLE_CLIENT_DISCONNECT(index, arg);
			break;
		default:
			break;
		}
		Rec = 1;
	}
	free(received);
	free(arg);
	return 0;
}

/// <summary>
/// Writing message to Log File
/// </summary>
/// <param name="hfile"></param>
/// <param name="data_to_write"></param>
/// <param name="bytes_to_write"></param>
/// <returns></returns>
int WriteData_David(HANDLE* hfile, char* data_to_write, DWORD bytes_to_write) {    /////// CHANGE THIS NAME 
	BOOL bErrorFlag;
	DWORD dwBytesWritten = 0;
	bErrorFlag = WriteFile(
		*hfile,             // open file handle
		data_to_write,		// start of data to write
		bytes_to_write,		// number of bytes to write
		&dwBytesWritten,	// number of bytes that were written
		NULL);				// no overlapped structure

	if (FALSE == bErrorFlag)
	{
		printf("WriteFile error: \n Terminal failure: Unable to write to file.\n");
		return 1;
	}
	else if (dwBytesWritten != bytes_to_write)
	{
		// This is an error because a synchronous write that results in
		// success (WriteFile returns TRUE) should write all data as
		// requested. This would not necessarily be the case for
		// asynchronous writes.
		printf("WriteFile error: \n dwBytesWritten != dwBytesToWrite\n");
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if ((StartGameEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL){
		printf("Couldn't create event\n");
		return 1;
	}
	if ((RestartGameEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL){
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
	if ((server_s = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	//if ((server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return 1;
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[1]));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int server_addrss_len = sizeof(server_addr);
	HANDLE RestartThread;
	char* exit_input = NULL;

	//Bind
	if (bind(server_s, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	SOCKET client_s;
	DWORD threadIDs[10];
	int received = 0;

	//wait for data to be sent by the channel WSA_FLAG_OVERLAPPED
	int ListenRes = listen(server_s, SOMAXCONN);
	if (ListenRes == SOCKET_ERROR)
	{
		printf("Failed listening on socket, error %ld.\n", WSAGetLastError());
		return CleanUpAll();
	}
	if ((GameStsteMutex = CreateMutex(NULL, NULL, NULL)) == NULL)
	{
		printf("Couldn't create thread\n");
		return CleanUpAll();
	}
	if (openThread(&RestartThread, &ResatartGame, NULL, &(threadIDs[0])))
	{
		openedsuccessfuly[0] = 0; // if thread wasnt opened successfully
		printf("Error with thread %d\n", 0);
	}
	while (1)
	{ 
		if (socketCount < 2)
		{
			client_s = accept(server_s, NULL, NULL);
			if (client_s == INVALID_SOCKET)
			{
				printf("Accepting connection with client failed, error %ld\n", WSAGetLastError());
			}
			allSockets[socketCount] = client_s;

			///// CHECK IF THERE ARE NO MORE THN 2 THREADS OR DENY
			char* recieved = NULL;
			if (openThread(&((Threads)[socketCount]), &threadExecute, &client_s, &(threadIDs[0])))
			{
				openedsuccessfuly[0] = 0; // if thread wasnt opened successfully
				printf("Error with thread %d\n", 0);
			}
			socketCount++;			
		}
		if (_kbhit())
		{//check if 'exit' was pressed to stdin. if so, send Ack to the channel, close the connection, and close the output file.

			exit_input = readinput();
			if ((!strcmp(exit_input, "exit")))
			{//////////////////////////////////////////////////////////////////////////// take care
				//finish execution
				break;
			}
		}
	}
	// Deinitialize Winsock
	closeProcess(RestartThread);
	CleanUpAll();
	return 0;
}
