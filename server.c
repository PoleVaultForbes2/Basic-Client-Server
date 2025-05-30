/*
  Josh Forbse
   Start Code from https://www.beej.us/guide/bgnet/html/#client-server-background"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 256
#define BACKLOG 10

int clients[MAX_CLIENTS] = {0};

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "server-binary-name filename port\n");
        exit(1);
    }

    char *filename = argv[1];
    char *PORT = argv[2];
    char buffer[BUFFER_SIZE];
    int listener, newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char remoteIP[INET6_ADDRSTRLEN];

    struct addrinfo hints, *ai, *p;
    int yes = 1, rv;

    fd_set master, read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // bind to first available
    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) continue;

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL) {
        perror("server: failed to bind");
        exit(2);
    }

    freeaddrinfo(ai);

    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        exit(3);
    }

    FD_SET(listener, &master);
    fdmax = listener;

    printf("server: waiting for connections...\n");

    while (1) {
        read_fds = master;

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    // new connection
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                        continue;
                    }

                    FD_SET(newfd, &master);
                    if (newfd > fdmax) fdmax = newfd;

                    // save client
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j] == 0) {
                            clients[j] = newfd;
                            break;
                        }
                    }

                    printf("server: new connection from %s on socket %d\n",
                        inet_ntop(remoteaddr.ss_family,
                            get_in_addr((struct sockaddr*)&remoteaddr),
                            remoteIP, INET6_ADDRSTRLEN), newfd);

                    // send file to client
                    FILE *file = fopen(filename, "r");
                    if (file) {
                        while (fgets(buffer, BUFFER_SIZE, file)) {
                            send(newfd, buffer, strlen(buffer), 0);
                        }
                        fclose(file);
                    }
                } else {
                    // data from existing client
                    memset(buffer, 0, BUFFER_SIZE);
                    int nbytes = recv(i, buffer, sizeof buffer, 0);

                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j] == i) clients[j] = 0;
                        }
                    } else {
                        // echo back and broadcast
                        send(i, buffer, nbytes, 0);
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j] != 0 && clients[j] != i) {
                                send(clients[j], buffer, nbytes, 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
