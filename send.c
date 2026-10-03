#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "common.h"



int sendAll(SOCKET sock, const char *buf, int len) {
    int totalSent = 0;

    while (totalSent < len) {
        int bytesSent = send(sock, buf + totalSent, len - totalSent, 0);

        if (bytesSent == SOCKET_ERROR) {
            return SOCKET_ERROR;  
        }

        totalSent += bytesSent;
    }

    return totalSent;  
}

uint64_t findSize(const char *file_name)
{
    FILE *fp = fopen(file_name, "rb");

    if (!fp) {
        printf("File not found!\n");
        return 0;
    }

    _fseeki64(fp, 0, SEEK_END);

    int64_t size = _ftelli64(fp);

    fclose(fp);

    if (size < 0)
        return 0;

    return (uint64_t)size;
}

int handel_send(char *path, char *address_ip, char *port){
    
    struct addrinfo hints = {0}, *result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddrinfo(address_ip, port, &hints, &result);

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        printf("couldn't create the socket!\n");
        WSACleanup();
        return 3;
    } 
    
    BOOL nodelay = TRUE;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay));

    int connect_result = connect(sock, result->ai_addr, (int)result->ai_addrlen);

    if(connect_result == SOCKET_ERROR){
        printf("couldn't connect! check the port or the address.");
        freeaddrinfo(result);
        closesocket(sock);
        WSACleanup();
        return 4;
    }
    printf("connected successfully!\n");

    const char *filename = strrchr(path, '\\');
    filename = filename ? filename + 1 : path;  // handle no backslash case

    uint32_t filename_len = (uint32_t)strlen(filename);
    uint32_t filename_len_net = htonl(filename_len);

    // send the length of the filename first
    if (sendAll(sock, (const char *)&filename_len_net, sizeof(filename_len_net)) == SOCKET_ERROR) {
        printf("failed to send filename length\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // then send the filename itself
    if (sendAll(sock, filename, filename_len) == SOCKET_ERROR) {
        printf("failed to send filename\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    FILE *file_to_send;
    file_to_send = fopen(path, "rb");

    if(!file_to_send){
        printf("no such file exists!\n");
        WSACleanup();
        return 2;
    }

    int64_t size = findSize(path);

    if (size < 0) {
        printf("couldn't determine file size\n");
        fclose(file_to_send);
        return 2;
    }

    uint64_t size_of_file = (uint64_t)size;


    if (sendAll(sock, (char *)&size_of_file, sizeof(size_of_file)) == SOCKET_ERROR) {
        printf("failed to send file size\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }


    size_t current = 0;

    ULONGLONG start = GetTickCount64();

    char buffer[BYTES_TO_SEND];
    size_t bytes_read = 0;
    while((bytes_read = fread(buffer, sizeof(char), BYTES_TO_SEND, file_to_send)) > 0){ 

        int result = sendAll(sock, buffer, (int)bytes_read);

        if (result == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        current += bytes_read;

        ULONGLONG elapsed_ms = GetTickCount64() - start;

        if (elapsed_ms > 0) {
            double elapsed = elapsed_ms / 1000.0;

            // IMPORTANT: bytes/second
            double speed = current / elapsed;

            progress_bar("sending", current, size_of_file, speed);
        }
        
    }  
    printf("file sent!");
    fclose(file_to_send);
    freeaddrinfo(result);
    closesocket(sock);
    WSACleanup();
    return 5;
}