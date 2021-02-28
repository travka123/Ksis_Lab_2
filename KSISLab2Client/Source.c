#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <locale.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_SEED 1

int GetNextRandomValue(int last)
{
	int rw = last * last * 97 + last;
	return (rw == 0) ? (rw++) : (rw);
}

WSADATA wsaData;
int pakegeForSendCount;

void InitWinsock()
{
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		exit(1);
	}
}

SOCKET SetConnection(int protocol)
{
	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	switch (protocol)
	{
	case IPPROTO_TCP:
		hints.ai_socktype = SOCK_STREAM;
		break;
	case IPPROTO_UDP:
		hints.ai_socktype = SOCK_DGRAM;
		break;
	}
	hints.ai_protocol = protocol;

	ADDRINFO* result;
	int iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;
	ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		exit(1);
	}
	return ConnectSocket;
}

void RandomSend(SOCKET ConnectSocket)
{
	int data = GetNextRandomValue(DEFAULT_SEED);
	for (int i = 0; i < pakegeForSendCount; i++)
	{
		send(ConnectSocket, &data, sizeof(int), 0);
		data = GetNextRandomValue(data);
	}
}

void CloseSocket(SOCKET ConnectSocket)
{
	int closesignal = 0;
	send(ConnectSocket, &closesignal, sizeof(int), 0);
	send(ConnectSocket, &closesignal, sizeof(int), 0);
	send(ConnectSocket, &closesignal, sizeof(int), 0);
	int iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	iResult = closesocket(ConnectSocket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"close failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
}


void SetPackegeCount(SOCKET ConnectSocket)
{
	send(ConnectSocket, &pakegeForSendCount, sizeof(int), 0);
}

int iResult;

int main()
{
	setlocale(LC_ALL, "Russian");
	InitWinsock();

	SOCKET ConnectSocket = SetConnection(IPPROTO_TCP);

	printf("¬ведите кол-во пакетов дл€ отправки: ");
	scanf_s("%d", &pakegeForSendCount);
	SetPackegeCount(ConnectSocket);

	RandomSend(ConnectSocket);
	CloseSocket(ConnectSocket);
	printf("TCP пакеты отправлены\n");
	system("pause");
	
	ConnectSocket = SetConnection(IPPROTO_UDP);
	RandomSend(ConnectSocket);
	CloseSocket(ConnectSocket);
	printf("UDP пакеты отправлены\n");

	WSACleanup();
	return 0;
}