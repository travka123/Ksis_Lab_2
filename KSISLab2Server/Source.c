#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <time.h> 
#include <locale.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_SEED 1

int GetNextRandomValue(int last)
{
	int rw = last * last * 97 + last;
	return (rw == 0) ? (rw++) : (rw);
}

WSADATA wsaData;

void InitWinsock()
{
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		exit(1);
	}
}

SOCKET SetListenSocket(int protocol)
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
	hints.ai_flags = AI_PASSIVE;

	ADDRINFO* result;
	int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		exit(1);
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
	freeaddrinfo(result);

	if ((protocol == IPPROTO_TCP) && listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(1);
	}
	return ListenSocket;
}

SOCKET WaitForClientSocket(SOCKET ListenSocket)
{
	SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	return ClientSocket;
}

typedef struct _DataTransferInfo
{
	int messageCount;
	int dataSize;
	int packageLost;
	time_t transferTime;
} DataTranserInfo;

DataTranserInfo Recive(SOCKET ClientSocket)
{
	DataTranserInfo dti;
	memset(&dti, 0, sizeof(dti));
	int expData = GetNextRandomValue(DEFAULT_SEED);
	int recData;
	int iResult;
	do
	{
		iResult = recv(ClientSocket, &recData, sizeof(int), 0);
		if (dti.messageCount == 0)
		{
			dti.transferTime = clock();
		}
		if (iResult > 0)
		{
			if (recData == 0)
			{
				dti.transferTime = clock() - dti.transferTime;
				return dti;
			}
			dti.messageCount++;
			dti.dataSize += iResult;
			if (recData != expData)
			{
				dti.packageLost++;
			}
			expData = GetNextRandomValue(recData);
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
	dti.transferTime = clock() - dti.transferTime;
	return dti;
}

int waitingPackageCount = 0;

void SetPackageCount(SOCKET s)
{
	recv(s, &waitingPackageCount, sizeof(int), 0);
}

void PrintDTInfo(const DataTranserInfo dti)
{
	printf("Сообщений получено: %d\n", dti.messageCount);
	printf("Повреждено пакетов: %d\n", dti.packageLost);
	printf("Потеряно пакетов: %d\n", waitingPackageCount - dti.messageCount);
	printf("Средняя скорость передачи: %lf Mbit/s\n", (long double)dti.dataSize * 8 / dti.transferTime * CLOCKS_PER_SEC / 1000 / 1000);
}

int main() {
	setlocale(LC_ALL, "Russian");
	InitWinsock(&wsaData);

	SOCKET ListenSocket = SetListenSocket(IPPROTO_TCP);
	SOCKET ClientSocket = WaitForClientSocket(ListenSocket);
	SetPackageCount(ClientSocket);
	DataTranserInfo TCPdti = Recive(ClientSocket);
	closesocket(ClientSocket);
	closesocket(ListenSocket);

	printf("TCP:\n");
	PrintDTInfo(TCPdti);

	ListenSocket = SetListenSocket(IPPROTO_UDP);
	DataTranserInfo UDPdti = Recive(ListenSocket);
	closesocket(ListenSocket);

	printf("\nUDP:\n");
	PrintDTInfo(UDPdti);

	WSACleanup();
	return 0;
}