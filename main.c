#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string.h>

#include "send.h"
#include "recv.h"

int loading_dll(){
    WSADATA wsadata;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsadata);

    if(wsaerr != 0){
        printf("The winsock dll not found");
        return 1;
    }
}

/*
test path: C:\Users\fechlou9\Documents\whql-amd-software-adrenalin-edition-26.5.2-win11-may-vega-polaris.exe
*/
//transfer -s -r -p 8888 -ip 127.0.0.1 -f file_path 
int main(int argc, char **argv){
    int decesion = -1;
    char *path_of_file = NULL;
    char *path_of_directory = NULL;
    char *ip = NULL;
    char *port = NULL;

    if(argc < 2){
        printf("not enough inputs!\n-s send\n-r receive\n-p port\n-ip IP address\n-f file path\n-d directory to store\n");
        return 1;
    } else {
        for(int i = 1; i < argc; i++){
            if(strcmp(argv[i], "-s") == 0){
                decesion = 1;
            } else if(strcmp(argv[i], "-r") == 0){
                decesion = 0;
            } else if(strcmp(argv[i], "-f") == 0){
                if(i + 1 < argc){
                    path_of_file = argv[i + 1];
                    i++;
                } else {
                    printf("no file specified with -f !\n");
                    return 1;
                }
            } else if(strcmp(argv[i], "-d") == 0){
                if(i + 1 < argc){
                    path_of_directory = argv[i + 1];
                    i++;
                } else {
                    printf("no directory specified with -d !\n");
                    return 1;
                }
            } else if(strcmp(argv[i], "-ip") == 0){
                if(i + 1 < argc){
                    ip = argv[i + 1];
                    i++;
                } else {
                    printf("no IP specified with -ip !\n");
                    return 1;
                }
            } else if(strcmp(argv[i], "-p") == 0){
                if(i + 1 < argc){
                    port = argv[i + 1];
                    i++;
                } else {
                    printf("no port specified with -p !\n");
                    return 1;
                }
            } else if(strcmp(argv[i], "-h")==0){
                printf("-s send\n-r receive\n-p port\n-ip IP address\n-f file path\n-d directory to store\n");
                return 1;
            }
        }

        // Post-loop validation, once all flags are collected
        if(decesion == -1){
            printf("must specify -s (send) or -r (receive)!\n");
            return 1;
        }
        if(decesion == 1 && path_of_file == NULL){
            printf("no file specified with -f !\n");
            return 1;
        }
        if(decesion == 0 && path_of_directory == NULL){
            printf("no directory specified with -d !\n");
            return 1;
        }
    }

    loading_dll();

    if(decesion == 1){
        handel_send(path_of_file, ip, port);
    } else {
        handel_recv(path_of_directory, port);
    }

    WSACleanup();
    return 0;
}