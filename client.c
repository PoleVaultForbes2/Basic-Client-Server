/*
   Josh Forbes
   Starter code from 
   https://www.beej.us/guide/bgnet/html/#client-server-background"
*/

#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAXDATASIZE 100

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nExiting gracefully...\n");
        system("stty sane");
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,"client-binary-name host port file\n");
        exit(1);
    }

    char *host = argv[1];
    char *PORT = argv[2];
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\nCan't catch SIGINT\n");

    system("stty cbreak -echo");

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(host, PORT, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        system("stty sane");
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        system("stty sane");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connected to %s\n", s);
    freeaddrinfo(servinfo);

    fd_set read_fds;
    int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) <= 0) {
                if (numbytes == 0) printf("Connection closed by server.\n");
                else perror("recv");
                break;
            }
            buf[numbytes] = '\0';
            printf("%s", buf);
            fflush(stdout);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input_buf[MAXDATASIZE];
            int n = 0;
            char c;

            while (n < MAXDATASIZE - 1 && read(STDIN_FILENO, &c, 1) == 1) {
                input_buf[n++] = c;
                if (c == '\033') { // arrow key (ESC)
                    read(STDIN_FILENO, &c, 1);
                    input_buf[n++] = c;
                    read(STDIN_FILENO, &c, 1);
                    input_buf[n++] = c;
                    break;
                }
                if (c == '\n' || c == '\r') break;
                if (input_buf[0] != '\033') break;
            }

            input_buf[n] = '\0';
            if (input_buf[0] == '.' && n == 1) break;
            send(sockfd, input_buf, n, 0);
        }
    }

    system("stty sane");
    close(sockfd);
    return 0;
}
