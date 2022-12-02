// Don't forget to include "wsock32" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <dirent.h>
#include <assert.h>
const int TIME_PORT = 12345;

// the checkForAnError function checks if there was an error and handles it accordingly
// @ params
// bytesResult = send/receive function result
// ErrorAt = string stating if it was receive or send function
// socket_1 & socket_2 = the sockets to close in case there was an error.

bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket_1, SOCKET socket_2){
    if (SOCKET_ERROR == bytesResult) {
        printf("Time Server: Error at %s(): ",ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket_1);
        closesocket(socket_2);
        WSACleanup();
        return true;
    }
    return false;
}

char *readFile(char *filename) {
    FILE *f = fopen(filename, "rt");
    assert(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *) malloc(length + 1);
    buffer[length] = '\0';
    fread(buffer, 1, length, f);
    fclose(f);
    return buffer;
}

void main() {
	WSADATA wsaData;
	SOCKET listenSocket;
	struct sockaddr_in serverService;
	char timeBuff[26];
	time_t timer;
	struct tm * tm_info;
	char randomletter;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData)) {
		printf("Time Server: Error at WSAStartup()\n");
		return;
	}

	listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket) {
		printf("Time Server: Error at socket(): ");
		printf("%d", WSAGetLastError());
		WSACleanup();
		return;
	}

	memset( & serverService, 0, sizeof(serverService));
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = htonl(INADDR_ANY);
	serverService.sin_port = htons(TIME_PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR * ) & serverService, sizeof(serverService))) {
		printf("Time Server: Error at bind(): ");
		printf("%d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5)) {
		printf("Time Server: Error at listen(): ");
		printf("%d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	while(1)
    {

        struct sockaddr_in from; // Address of sending partner
        int fromLen = sizeof(from);

        printf("Time Server: Wait for clients' requests.\n");

        SOCKET msgSocket = accept(listenSocket, (struct sockaddr * ) & from, & fromLen);
        if (INVALID_SOCKET == msgSocket) {
            printf("Time Server: Error at accept(): ");
            printf("%d", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        printf("Time Server: Client is connected.\n");
        while (1) {
            int bytesSent = 0;
            int bytesRecv = 0;
            char * sendBuff;
            char recvBuff[255];

            bytesRecv = recv(msgSocket, recvBuff, 255, 0);
            if (checkForAnError(bytesRecv,"recv",listenSocket,msgSocket))
                return;
            char* request = strtok(recvBuff," ");
            char* file = strtok(NULL," ");
            printf("request: %s\nfile: %s\n\n", request,file);
            if (strncmp(recvBuff, "listfolder", 10) == 0) {
                DIR *d;
                struct dirent *dir;
                char directory[100] = "";
                int ignoreCount = 0;
                d = opendir("files");
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                            if(ignoreCount>=2){
                                strcat(directory,dir->d_name);
                                strcat(directory," ");
                            }
                        ignoreCount++;
                    }
                    closedir(d);
                }
                sendBuff = directory;
            }
            else if (strncmp(request, "getfile", 7) == 0) {
                char pathToFile[100] = "files\\";
                strcat(pathToFile,file);
                char* content = readFile(pathToFile);
                printf("content: %s",content);
                sendBuff = content;
                strcpy(file,"");
                memset(file,0,sizeof file);
            }
            else if (strncmp(request, "calcrtt", 7) == 0) {
                //Sleep(3000);
                sendBuff = "My Name is Time Server, Nice to meet you.";
            }
            else{
                printf("Time Server: Closing Connection.\n");
                closesocket(msgSocket);
                break;
            }

            //printf("Sending: %s\n",sendBuff);
            bytesSent = send(msgSocket, sendBuff, (int) strlen(sendBuff), 0);
            if (checkForAnError(bytesRecv,"send",listenSocket,msgSocket))
                return;

            fflush(stdin);
            sendBuff="";
        }
    }
    closesocket(listenSocket);
    WSACleanup();
    return;
}
