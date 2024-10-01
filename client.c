#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

void* client_handler(void* arg);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_clients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    pthread_t clients[n];

    // generate n simulatenous clients
    for (int i = 0; i < n; i++) {
        pthread_create(&clients[i], NULL, client_handler, NULL);
    }
    // Wait for each client
    for (int i = 0; i < n; i++) {
        pthread_join(clients[i], NULL);
    }

    return 0;
}

void* client_handler(void* arg) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[2048] = {0}; // used to store the response from the server 

    // make a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // conver the ip addresses 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return NULL;
    }

    // make connection to the server 
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return NULL;
    }
    send(sock, "Get CPU Info", strlen("Get CPU Info"), 0);
    printf("Request sent to server...\n");

    // Receive and print the server's response
    read(sock, buffer, sizeof(buffer));
    printf("Recieved message from the server is :\n%s\n", buffer);

    close(sock);
    return NULL;
}