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
const int PROXY_PORT = 12345;
const int MAIN_PORT = 12346;

// the checkForAnError function checks if there was an error and handles it accordingly
// @ params
// bytesResult = send/receive function result
// ErrorAt = string stating if it was receive or send function
// socket_1 & socket_2 = the sockets to close in case there was an error.

bool checkForAnError(int bytesResult, char* ErrorAt, SOCKET socket_1, SOCKET socket_2)
{
    if (SOCKET_ERROR == bytesResult)
    {
        printf("Proxy Server: Error at %s(): ",ErrorAt);
        printf("%d", WSAGetLastError());
        closesocket(socket_1);
        closesocket(socket_2);
        WSACleanup();
        return true;
    }
    return false;
}

bool checkForAnErrorMain(int bytesResult, char* ErrorAt, SOCKET socket_1)
{
    if (SOCKET_ERROR == bytesResult)
    {
        printf("Proxy Server: Error at %s(): ",ErrorAt);
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

#include <stdio.h>
#include <stdbool.h>

// return true if the file specified by the filename exists
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

int main()
{
    WSADATA wsaData;
    SOCKET listenSocket;
    struct sockaddr_in serverService;
    char timeBuff[26];
    time_t timer;
    struct tm * tm_info;
    char randomletter;

    if (NO_ERROR != WSAStartup(MAKEWORD(2, 0), & wsaData))
    {
        printf("Proxy Server: Error at WSAStartup()\n");
        return 1;
    }

    listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (INVALID_SOCKET == listenSocket)
    {
        printf("Proxy Server: Error at socket(): ");
        printf("%d", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    memset( & serverService, 0, sizeof(serverService));
    serverService.sin_family = AF_INET;
    serverService.sin_addr.s_addr = htonl(INADDR_ANY);
    serverService.sin_port = htons(PROXY_PORT);

    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR * ) & serverService, sizeof(serverService)))
    {
        printf("Proxy Server: Error at bind(): ");
        printf("%d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (SOCKET_ERROR == listen(listenSocket, 5))
    {
        printf("Proxy Server: Error at listen(): ");
        printf("%d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    while(1)
    {

        struct sockaddr_in from; // Address of sending partner
        int fromLen = sizeof(from);

        printf("Proxy Server: Wait for clients' requests.\n");

        SOCKET msgSocket = accept(listenSocket, (struct sockaddr * ) & from, & fromLen);
        if (INVALID_SOCKET == msgSocket)
        {
            printf("Proxy Server: Error at accept(): ");
            printf("%d", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        printf("Proxy Server: Client is connected.\n");
        while (1)
        {
            int bytesSent = 0;
            int bytesRecv = 0;
            char * sendBuff;
            char recvBuff[255] = "";
            char* request = "";
            char* file = "";
            char fullRequest[255] = "";

            bytesRecv = recv(msgSocket, recvBuff, 255, 0);
            if (checkForAnError(bytesRecv,"recv",listenSocket,msgSocket))
                return 1;
            strcpy(fullRequest,recvBuff);
            request = strtok(recvBuff," ");
            file = strtok(NULL," ");
            printf("\nrequest: %s\nfile: %s\n\n", request,file);
            if (strncmp(request, "listfolder", 10) == 0)
            {
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
            else if (strncmp(request, "getfile", 7) == 0)
            {
                int fileLength = strlen(file) + 7;
                char *pathToFile = (char*) malloc(fileLength*sizeof(char));
                strcpy(pathToFile,"files\\");
                strcat(pathToFile,file);
                if(fileExists(pathToFile)){
                    printf("File %s found, sending: \n", file);
                    char* content = readFile(pathToFile);
                    printf("content: %s",content);
                    sendBuff = content;
                    content="";
                } else {
                    printf("File %s not found, forwarding request to main server.\n", file);

                    SOCKET mainConnSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (INVALID_SOCKET == mainConnSocket) {
                        printf("Proxy Client: Error at socket(): ");
                        printf("%d", WSAGetLastError());
                        WSACleanup();
                        return 1;
                    }

                    struct sockaddr_in mainServer;
                    memset( & mainServer, 0, sizeof(mainServer));
                    mainServer.sin_family = AF_INET;
                    mainServer.sin_addr.s_addr = inet_addr("127.0.0.1");
                    mainServer.sin_port = htons(MAIN_PORT);

                    if (SOCKET_ERROR == connect(mainConnSocket, (SOCKADDR * ) & mainServer, sizeof(mainServer))) {
                        printf("Proxy Client: Error at connect(): ");
                        printf("%d", WSAGetLastError());
                        closesocket(mainConnSocket);
                        WSACleanup();
                        return 1;
                    }
                    int mainBytesSent = 0;
                    int mainBytesRecv = 0;
                    char *sendMainBuff;
                    char recvMainBuff[255] = "";

                    sendMainBuff = fullRequest;

                    mainBytesSent = send(mainConnSocket, sendMainBuff, (int) strlen(sendMainBuff), 0);
                    if (checkForAnErrorMain(mainBytesSent,"send",mainConnSocket))
                        return 1;

                    mainBytesRecv = recv(mainConnSocket, recvMainBuff, sizeof recvMainBuff, 0);
                    if (checkForAnErrorMain(mainBytesRecv,"recv",mainConnSocket))
                        return 1;
                    //closesocket(mainConnSocket);
                    printf("Recieved from main server:\n%s\n",recvMainBuff);
                    if(strncmp(recvMainBuff,"fileNotFound",12) == 0){
                        sendBuff="Requested file was not found";
                    } else {
                        FILE *f = fopen(pathToFile, "w");
                        if(f==NULL){
                            printf("FOPEN: Error has occured");
                            return 1;
                        }
                        fprintf(f,"%s", recvMainBuff);
                        fclose(f);
                        sendBuff = recvMainBuff;
                    }

                }
                file="";
                free(pathToFile);
            }
            else
            {
                printf("Proxy Server: Closing Connection.\n");
                closesocket(msgSocket);
                break;
            }

            //printf("Sending: %s\n",sendBuff);
            bytesSent = send(msgSocket, sendBuff, (int) strlen(sendBuff), 0);
            if (checkForAnError(bytesRecv,"send",listenSocket,msgSocket))
                return 1;

            fflush(stdin);
            sendBuff="";
        }
    }
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
