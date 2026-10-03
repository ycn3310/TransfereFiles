#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdint.h>

#include "common.h"


int recvAll(SOCKET sock, char *buf, int len) {
    int totalReceived = 0;

    while (totalReceived < len) {
        int bytesReceived = recv(sock, buf + totalReceived, len - totalReceived, 0);

        if (bytesReceived == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        if (bytesReceived == 0) {
            break; 
        }

        totalReceived += bytesReceived;
    }

    return totalReceived;
}

int handel_recv(char *save_path, char *port){     
    struct addrinfo hints = {0}, *result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ai_result = getaddrinfo(NULL, port, &hints, &result);
    if (ai_result != 0) {
        printf("getaddrinfo failed: %d\n", ai_result);
        WSACleanup();
        return 3;
    }
    
    SOCKET listen_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    BOOL nodelay = TRUE;
    setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay));

    if (listen_sock == INVALID_SOCKET) {
        printf("couldn't create the socket!\n");
        freeaddrinfo(result);
        WSACleanup();
        return 3;
    }

    if(bind(listen_sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
        printf("Couldn't bind the socket! Error: %s", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listen_sock);
        WSACleanup();
        return 4;
    }
    freeaddrinfo(result);

    if(listen(listen_sock, SOMAXCONN) == SOCKET_ERROR){
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 5;
    }

    printf("waiting for connection...\n");
    SOCKET client_socket = accept(listen_sock, NULL, NULL);

    if(client_socket == INVALID_SOCKET){
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 6;
    }

    uint32_t filename_len_net;
    if (recvAll(client_socket, (char *)&filename_len_net, sizeof(filename_len_net)) != sizeof(filename_len_net)) {
        printf("failed to receive filename length\n");
        closesocket(client_socket);
        closesocket(listen_sock);
        WSACleanup();
        return 2;
    }
    uint32_t filename_len = ntohl(filename_len_net);

    // sanity check — don't trust the network blindly
    if (filename_len == 0 || filename_len >= MAX_PATH) {
        printf("invalid filename length received\n");
        closesocket(client_socket);
        closesocket(listen_sock);
        WSACleanup();
        return 2;
    }

    // read the filename itself
    char received_filename[MAX_PATH] = {0};
    if (recvAll(client_socket, received_filename, filename_len) != (int)filename_len) {
        printf("failed to receive filename\n");
        closesocket(client_socket);
        closesocket(listen_sock);
        WSACleanup();
        return 2;
    }
    received_filename[filename_len] = '\0';

    uint64_t size_of_file;
    if (recvAll(client_socket, (char *)&size_of_file, sizeof(size_of_file)) != sizeof(size_of_file)) {
        printf("failed to receive filename\n");
        closesocket(client_socket);
        closesocket(listen_sock);
        WSACleanup();
        return 2;
    }

    printf("receiving file: %s\n", received_filename);

    char full_save_path[MAX_PATH];
    snprintf(full_save_path, sizeof(full_save_path), "%s\\%s", save_path, received_filename);

    FILE *output_file = fopen(full_save_path, "wb");
        

    char buffer[BYTES_TO_RECEIVE];
    int bytesReceived;
    uint64_t current=0;
    ULONGLONG start = GetTickCount64();
    do {
        bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
        ULONGLONG elapsed_ms = GetTickCount64() - start;
        if (elapsed_ms > 0) {
            double elapsed = elapsed_ms / 1000.0;

            // IMPORTANT: bytes/second
            double speed = current / elapsed;

            progress_bar("downoading", current, size_of_file, speed);
        }

        if (bytesReceived > 0) {
            fwrite(buffer, 1,  bytesReceived, output_file);
        } else if (bytesReceived == 0) {
            printf("Connection closed.\n");
        } else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }
        current += bytesReceived;
        
    } while (bytesReceived > 0);

    fclose(output_file);
    closesocket(client_socket);
    closesocket(listen_sock);
    WSACleanup();
}