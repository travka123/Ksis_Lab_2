#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h> 

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_SEED 1

int GetNextRandomValue(int last)
{
	return last * last * 97 + last;
}

int iResult;

int main() {
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	ADDRINFO* result;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET ClientSocket = INVALID_SOCKET;

	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	int recData;
	int countTCP = 0;
	time_t time;
	time = clock();
	do
	{
		iResult = recv(ClientSocket, &recData, sizeof(int), 0);
		if (iResult > 0)
		{
			countTCP++;
		}
		else if (iResult == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
		}
	} while (iResult > 0);
	closesocket(ClientSocket);
	closesocket(ListenSocket);
	//////////////////////////////////////////////

	double secondsTCP = (double)(clock() - time) / CLOCKS_PER_SEC;
	double millisecondsTCP = secondsTCP * 1000;
	printf("\n");
	printf("TCP messages count: %d\n", countTCP);
	printf("TCP transfer time: %f milliseconds\n", millisecondsTCP);
	printf("TCP message size: %d\n", sizeof(recData));
	printf("TCP speed: %f Mbit/s\n", (double)countTCP * sizeof(recData) / 1024 / 1024 / secondsTCP);
	return 0;
}