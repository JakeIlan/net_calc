#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define PORT 12345 //Порт сервера
#define IP_SERVER "127.0.0.1" //Адрес сервера
#define SIZE_MSG 100


int readN(int socket, char *buf);


int main(int argc, char **argv) {


    struct sockaddr_in peer;
    peer.sin_family = AF_INET;

    char inputBuf[SIZE_MSG];
    int sock = -1;
    int rc = -1;

    for (;;) {

        peer.sin_addr.s_addr = inet_addr(IP_SERVER);

        peer.sin_port = htons(PORT);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            printf("ERROR: Can't create socket! Try again.\n");
            fflush(stdout);
            break;
        }

        int rc = connect(sock, (struct sockaddr *) &peer, sizeof(peer));
        if (rc == -1) {
            perror("ERROR: Can't connect to server! Try again.\n");
            fflush(stdout);
            break;
        } else {
            printf("Connected.\n");
            fflush(stdout);
            break;
        }
    }

    printf("OK LET'S DO SOME MATH\n");
    fflush(stdout);
    char *tar;
    char *arg1;
    char *operation;
    char *arg2;

    char msg[SIZE_MSG] = {0};
    char msg_buf[SIZE_MSG] = {0};
    printf("Input (\'/help\' to help): \n");
    fflush(stdout);
    for (;;) {
        printf("[CALC]:");
        fgets(inputBuf, sizeof(inputBuf), stdin);
        inputBuf[strlen(inputBuf) - 1] = '\0';

        if (!strcmp("/help", inputBuf)) {
            printf("HELP:\n");
            printf("\'/quit or /q\' to leave after paying receipt\n");
            fflush(stdout);
        } else if (!strcmp("/q", inputBuf) || !strcmp("/quit", inputBuf)) {
            shutdown(sock, 2);
            break;
        } else {

            strcpy(msg_buf, inputBuf);
            char *sep = " ";
            char *str = strtok(msg_buf, sep);
            if (str == NULL) {
                printf("Wrong syntax! Try /help to see command menu.\n");
                fflush(stdout);
                continue;
            } else if (!strcmp("/f", str) || !strcmp("/s", str)) {
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int pay = atoi(str);

                    if (str[0] != '0' && pay == 0) {
                        printf("Illegal format! Use /pay NUMBER.\n");
                        fflush(stdout);
                        continue;
                    }
                    send(sock, inputBuf, sizeof(inputBuf), 0);
                    if (readN(sock, msg) <= 0) {
                        printf("Out of service...\n");
                        fflush(stdout);
                        close(sock);
                        break;
                    } else {
                        printf("%s\n", msg);
                        fflush(stdout);
                    }
                    if (readN(sock, msg) <= 0) {
                        printf("Out of service...\n");
                        fflush(stdout);
                        close(sock);
                        break;
                    } else {
                        printf("%s\n", msg);
                        fflush(stdout);
                    }
                }
            } else {
                send(sock, inputBuf, sizeof(inputBuf), 0);
                if (readN(sock, msg) <= 0) {
                    printf("Out of service...\n");
                    fflush(stdout);
                    close(sock);
                    break;
                } else {
                    printf("%s\n", msg);
                    fflush(stdout);
                }
            }

        }
        memset(inputBuf, 0, 100);
    }


    printf("ENDED CLIENT!\n");
    fflush(stdout);

    return 0;
}

int readN(int socket, char *buf) {
    int result = 0;
    int readBytes = 0;
    int sizeMsg = SIZE_MSG;
    while (sizeMsg > 0) {
        readBytes = recv(socket, buf + result, sizeMsg, 0);
        if (readBytes <= 0) {
            return -1;
        }
        result += readBytes;
        sizeMsg -= readBytes;
    }
    return result;
}
