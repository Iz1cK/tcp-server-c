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
const int MAIN_PORT = 12346;

// the checkForAnError function checks if there was an error and handles it accordingly
// @ params
// bytesResult = send/receive function result
// ErrorAt = string stating if it was receive or send function
// socket_1 & socket_2 = the sockets to close in case there was an error.

bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket_1, SOCKET socket_2){
    if (SOCKET_ERROR == bytesResult) {
        printf("Main Server: Error at %s(): ",ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket_1);
        closesocket(socket_2);
        WSACleanup();
        return true;
    }
    return false;
}

bool checkForAnErrorMain(int bytesResult, char* ErrorAt, SOCKET socket_1){
    if (SOCKET_ERROR == bytesResult) {
        printf("Main Server: Error at %s(): ",ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket_1);
        WSACleanup();
        return true;
    }
    return false;
}

char *readFile(char *filename)
{
    FILE *f = fopen(filename, "r");
    assert(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *) calloc(1,length + 1);
//    char *buffer = (char *) malloc(length + 1);
    buffer[length] = '\0';
//    memset(buffer,0,sizeof buffer);
    fread(buffer, 1, length, f);
//    printf("buffer: %s",buffer);
    fclose(f);
    return buffer;
}

bool fileExists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp != NULL)
    {
        fclose(fp);
        return true;
    }
    return false;
}

int main() {
	WSADATA wsaData;
	SOCKET listenSocket;
	struct sockaddr_in serverService;
	char timeBuff[26];
	time_t timer;
	struct tm * tm_info;
	char randomletter;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData)) {
		printf("Main Server: Error at WSAStartup()\n");
		return 1;
	}

	listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket) {
		printf("Main Server: Error at socket(): ");
		printf("%d", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	memset( & serverService, 0, sizeof(serverService));
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = htonl(INADDR_ANY);
	serverService.sin_port = htons(MAIN_PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR * ) & serverService, sizeof(serverService))) {
		printf("Main Server: Error at bind(): ");
		printf("%d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5)) {
		printf("Main Server: Error at listen(): ");
		printf("%d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	while(1)
    {

        struct sockaddr_in from; // Address of sending partner
        int fromLen = sizeof(from);

        printf("Main Server: Wait for Proxy's requests.\n");

        SOCKET msgSocket = accept(listenSocket, (struct sockaddr * ) & from, & fromLen);
        if (INVALID_SOCKET == msgSocket) {
            printf("Main Server: Error at accept(): ");
            printf("%d", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        printf("Main Server: Proxy is connected.\n");
        while (1) {
            int bytesSent = 0;
            int bytesRecv = 0;
            char * sendBuff;
            char recvBuff[255] = "";
            char fullRequest[255] = "";
            char* request = "";
            char* file = "";

            bytesRecv = recv(msgSocket, recvBuff, 255, 0);
            if (checkForAnError(bytesRecv,"recv",listenSocket,msgSocket))
                return 1;
            printf("Recieved Request From Proxy\n");
            strcpy(fullRequest,recvBuff);
            request = strtok(recvBuff," ");
            file = strtok(NULL," ");
            printf("request: %s\nfile: %s\n\n", request,file);
            if (strncmp(request, "getfile", 7) == 0) {
                int fileLength = strlen(file) + 7;
                char *pathToFile = (char*) malloc(fileLength*sizeof(char));
                strcpy(pathToFile,"files\\");
                strcat(pathToFile,file);
                if(fileExists(pathToFile)){
                    char* content = readFile(pathToFile);
                    printf("content: %s",content);
                    sendBuff = content;
                    content="";
                } else {
                    sendBuff="fileNotFound";
                }
                file="";
                free(pathToFile);
            }
            else{
                printf("Main Server: Closing Connection With Proxy.\n");
                closesocket(msgSocket);
                break;
            }

            printf("Sending: %s\n",sendBuff);
            bytesSent = send(msgSocket, sendBuff, (int) strlen(sendBuff), 0);
            if (checkForAnError(bytesRecv,"send",listenSocket,msgSocket))
                return 1;

            fflush(stdin);
            sendBuff="";
        }
    }
    closesocket(listenSocket);
    WSACleanup();
    return 1;
}
