#pragma once

char* PrepareMessage(int messageType, char* arg1);

int createAndSendMessage(int messageType, char* arg1, SOCKET client_s);

int SendBuffer(const char* Buffer, int BytesToSend, SOCKET sd);

int SendString(const char* Str, SOCKET sd);

int  ReceiveBuffer(char* OutputBuffer, int BytesToReceive, SOCKET sd);

int ReceiveString(char** OutputStrPtr, SOCKET sd);

int RecvDataThread(char** AcceptedStr, SOCKET client_s);

