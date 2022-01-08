#pragma once

int ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd);

int ReceiveString(char** OutputStrPtr, SOCKET sd);

int RecvDataThread(char** AcceptedStr, SOCKET client_s);

char* PrepareMessage(int messageType, char* arg1, char* arg2, char* arg3);

int SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);

int SendString(const char* Str, SOCKET sd);

int createAndSendMessage(SOCKET client_s, int messageType, char* arg1, char* arg2, char* arg3);

