/* Don't forget to include "wsock32" in the library list. */

#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

const int TIME_PORT = 12345;

bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket){
    if (SOCKET_ERROR == bytesResult) {
        printf("Time Client: Error at %s(): ",ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket);
        WSACleanup();
        return true;
    }
    return false;
}

void main() {
	WSADATA wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData)) {
		printf("Time Client: Error at WSAStartup()\n");
		return;
	}

	SOCKET connSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == connSocket) {
		printf("Time Client: Error at socket(): ");
		printf("%d", WSAGetLastError());
		WSACleanup();
		return;
	}

	struct sockaddr_in server;
	memset( & server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(TIME_PORT);

	if (SOCKET_ERROR == connect(connSocket, (SOCKADDR * ) & server, sizeof(server))) {
		printf("Time Client: Error at connect(): ");
		printf("%d", WSAGetLastError());
		closesocket(connSocket);
		WSACleanup();
		return;
	}
	printf("connection established successfully.");

	int bytesSent = 0;
	int bytesRecv = 0;
	char * sendBuff;
	char recvBuff[255];
	char option;

	while (option != '4') {
        bool three = false;

        printf("\nPlease insert an option :\n");
        printf("\n 1 : Get all file names in files folder");
        printf("\n 2 : Get a file from the files folder");
        printf("\n 3 : Calculate RTT");
        printf("\n 4 : Exit.");
        printf("\n Your option : ");
        scanf("%c", & option);

        switch(option){
            case '1':
                sendBuff = "listfolder ";
                break;
            case '2':
                printf("Please enter a file name:\n");
                char filename[100];
                scanf("%s",&filename);
                char request[100] = "getfile ";
                printf("%s", request);
                strcat(request,filename);
                sendBuff = request;
                break;
            case '3':
                printf("Calculating RTT:\n");
                double start = GetTickCount();
                sendBuff = "calcrtt ";
                bytesSent = send(connSocket, sendBuff, (int) strlen(sendBuff), 0);
                if (checkForAnError(bytesSent,"send",connSocket))
                    return;

                bytesRecv = recv(connSocket, recvBuff, 255, 0);
                if (checkForAnError(bytesRecv,"recv",connSocket))
                    return;
                double end = GetTickCount();
                three = true;
                printf("TTR: %f", end-start);
                break;
            case '4':
                sendBuff = "kill ";
                break;
            default:
                printf("\n *-*-* Please enter a valid option only. *-*-*\n");
                fflush(stdin);
                continue;
        }
        if(three) continue;
        bytesSent = send(connSocket, sendBuff, (int) strlen(sendBuff), 0);
        if (checkForAnError(bytesSent,"send",connSocket))
            return;
		if(option == '4'){
                printf("Closing connection.");
                break;
		}

		bytesRecv = recv(connSocket, recvBuff, 255, 0);
        if (checkForAnError(bytesRecv,"recv",connSocket))
			return;

		printf("\nRecieved from server: %s \n",recvBuff);
		fflush(stdin);
		sendBuff="";
        memset(recvBuff, 0, 255);
	}
}
