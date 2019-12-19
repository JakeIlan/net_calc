#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define PORT 1234 //Порт сервера
#define SIZE_MSG 100

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct tInfo {
    pthread_t threadId;
    char *address;
    int port;
    int socket;
    int number;
} *clients;


int clientQuantity = 0;
int operations = 0;

void *clientHandler(void *args);

int getNumber(char *tar, double *res);

u_int64_t factorial(int n);

void *clientTimer(void *args);

void *connectionListener(void *args);


void *kickClient(int kickNum);

int readN(int socket, char *buf);


int main(int argc, char **argv) {

//Серверный сокет
    struct sockaddr_in listenerInfo;
    listenerInfo.sin_family = AF_INET;
    listenerInfo.sin_port = htons(PORT);
    listenerInfo.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY INADDR_LOOPBACK

    int listener = socket(AF_INET, SOCK_STREAM, 0); //Прослушивающий сокет
    if (listener < 0) {
        perror("Can't create socket to listen: ");
        exit(1);
    }
    printf("Прослушивающий сокет создан.\n");
    fflush(stdout);

    int resBind = bind(listener, (struct sockaddr *) &listenerInfo, sizeof(listenerInfo));
    if (resBind < 0) {
        perror("Can't bind socket");
        exit(1);
    }
    printf("Прослушивающий сокет забинден.\n");
    fflush(stdout);

    if (listen(listener, 2)) { //Слушаем входящие соединения
        perror("Error while listening: ");
        exit(1);
    }
    printf("Начинаем слушать входящие соединения...\n");
    fflush(stdout);
//------------------------------------------------------

//Создание потока, который будет принимать входящие запросы на соединение
    pthread_t listenerThread;
    if (pthread_create(&listenerThread, NULL, connectionListener, (void *) &listener)) {
        printf("ERROR: Can't create listener thread!");
        fflush(stdout);
        exit(1);
    }

//Цикл чтения ввода с клавиатуры
    printf("Input (/help to help): \n");
    fflush(stdout);
    char buf[100];
    for (;;) {
        bzero(buf, 100);
        fgets(buf, 100, stdin);
        buf[strlen(buf) - 1] = '\0';

        if (!strcmp("/help", buf)) {
            printf("HELP:\n");
            printf("\'/lc' to view current clients\n");
            printf("\'/kick NUM' to kick NUM'th client\n");
            printf("\'/quit or /q\' to quit\n");
            fflush(stdout);
        } else if (!strcmp("/lc", buf)) {
            printf("Clients on-line:\n");
            printf(" NUMBER    ADDRESS         PORT\n");

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < clientQuantity; i++) {
                if (clients[i].socket != -1)
                    printf("  %d       %s        %d\n", clients[i].number, clients[i].address, clients[i].port);
            }
            pthread_mutex_unlock(&mutex);

            fflush(stdout);
        } else if (!strcmp("/quit", buf) || !strcmp("/q", buf)) {
            shutdown(listener, 2);
            close(listener);
            pthread_join(listenerThread, NULL);
            break;
        } else {
            char *sep = " ";
            char *str = strtok(buf, sep);
            if (str == NULL) {
                printf("Illegal format!\n");
                fflush(stdout);
                continue;
            }
            if (!strcmp("/kick", str)) {
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int kickNum = atoi(str);

                    if (str[0] != '0' && kickNum == 0) {
                        printf("Illegal format! Use /kick NUMBER.\n");
                        fflush(stdout);
                        continue;
                    }
                    kickClient(kickNum);
                }
            }
        }

    }

    printf("ENDED SERVER!\n");
    fflush(stdout);
    free(clients);
    return 0;
}

//Обработка одного клиента
void *clientHandler(void *args) {

    pthread_mutex_lock(&mutex);
    int index = *((int *) args);
    int sock = clients[index].socket;
    pthread_mutex_unlock(&mutex);

    char outMsg[SIZE_MSG] = {0};
    char msg[SIZE_MSG] = {0};
    for (;;) {
        if (readN(sock, msg) <= 0) {
            printf("Client №%d disconnect\n", index);
            fflush(stdout);
            shutdown(sock, 2);
            close(sock);
            pthread_mutex_lock(&mutex);
            clients[index].socket = -1;
            pthread_mutex_unlock(&mutex);
            break;

        } else {
            char *sep = " ";
            char *str = strtok(msg, sep);
            if (str == NULL) {
                printf("Undefined message from client %d.\n", index);
                fflush(stdout);
                continue;
            }
            if (!strcmp("/f", msg) || !strcmp("/s", msg)) {
                int opcode = 0;
                if (!strcmp("/s", msg)) {
                    opcode = 1;
                }
                str = strtok(NULL, sep);
                if (str != NULL) {
                    int arg = atoi(str);

                    if (str[0] != '0' && arg == 0) {
                        strcpy(outMsg, "Dude, you have to /pay NUMBER.\n");
                        send(sock, outMsg, sizeof(outMsg), 0);
                        continue;
                    }


                    if (arg <= 0) { // we aint no fools righ here
                        strcpy(outMsg, "OPERATION ERROR:");
                        send(sock, outMsg, sizeof(outMsg), 0);
                        strcpy(outMsg, "Your argument is below zero");
                        send(sock, outMsg, sizeof(outMsg), 0);
                        printf("Client %d has smol brain\n", index);
                        continue;
                    }


                    strcpy(outMsg, "Calculating.."); // ok we good
                    send(sock, outMsg, sizeof(outMsg), 0);
                    sleep(5);

                    pthread_mutex_lock(&mutex);

                    switch (opcode) {
                        case 0: {
                            printf("Received factorial of %d from client %d.\n", arg, index);
                            u_int64_t result = factorial(arg);
                            snprintf(outMsg, SIZE_MSG, "Calculation complete.\nYour result: %lu", result);
                            break;
                        }
                        case 1: {
                            printf("Received square root of %d from client %d.\n", arg, index);
                            double_t result = sqrt(arg);
                            snprintf(outMsg, SIZE_MSG, "Calculation complete.\nYour result: %.2f", result);
                            break;
                        }
                        default:
                            break;
                    }


                    send(sock, outMsg, sizeof(outMsg), 0);
                    pthread_mutex_unlock(&mutex);
                    fflush(stdout);
                    continue;
                }

            } else {
                printf("Parsing %s\n", msg);
                char *tar = msg;
                int cnt;
                int k = 1;
                double arg1 = 0, arg2 = 0;
                double result;

                while (*tar != '\r' && *tar != '\0') {

//                    cnt = getNumber(tar, &arg1);
                    if (*tar == '-') {
                        printf("Found negative mult\n");
                        k = -1;
                        tar++;
                        printf("Current target char %c\n", *tar);
                    }
                    if ((cnt = getNumber(tar, &arg1)) <= 0) {
                        strcpy(outMsg, "No first argument\n");
                        send(sock, outMsg, sizeof(outMsg), 0);
                        *tar = '\0';
                        break;
                    }
                    arg1 *= k;
                    tar += cnt;
                    printf("Total arg1 = %f\n", arg1);

                    switch (*tar) {
                        case '+': {
                            printf("Found +\n");
                            tar++;
                            if ((cnt = getNumber(tar, &arg2)) <= 0) {
                                printf("No second argument\n");
                                strcpy(outMsg, "No second argument\n");
                                *tar = '\0';
                                break;
                            }

                            tar += cnt;
                            printf("Total arg2 = %f\n", arg2);
                            result = arg1 + arg2;
                            printf("Result : %f + %f = %f\n", arg1, arg2, result);
                            snprintf(outMsg, SIZE_MSG, "Result : %.2f + %.2f = %.2f\n", arg1, arg2, result);
                            break;
                        }
                        case '-': {
                            printf("Found -\n");
                            tar++;
                            if ((cnt = getNumber(tar, &arg2)) <= 0) {
                                printf("No second argument\n");
                                strcpy(outMsg, "No second argument\n");
                                *tar = '\0';
                                break;
                            }

                            tar += cnt;
                            printf("Total arg2 = %f\n", arg2);
                            result = arg1 - arg2;
                            printf("Result : %f - %f = %f\n", arg1, arg2, result);
                            snprintf(outMsg, SIZE_MSG, "Result : %.2f - %.2f = %.2f\n", arg1, arg2, result);
                            break;
                        }
                        case '*': {
                            printf("Found *\n");
                            tar++;
                            k = 1;

                            if (*tar == '-') {
                                printf("Found negative mult\n");
                                k = -1;
                                tar++;
                                printf("Current target char %c\n", *tar);
                            }
                            if ((cnt = getNumber(tar, &arg2)) <= 0) {
                                printf("No second argument\n");
                                strcpy(outMsg, "No second argument\n");
                                *tar = '\0';
                                break;
                            }

                            tar += cnt;
//                            printf("Arg2 = %.2f\n", arg2);
                            arg2 *= k;
//                            printf("Arg2 * k = %.2f\n", arg2);
//                            printf("Total arg2 = %f\n", arg2);
                            result = arg1 * arg2;
                            printf("Result : %f * %f = %f\n", arg1, arg2, result);
                            snprintf(outMsg, SIZE_MSG, "Result : %.2f * %.2f = %.2f\n", arg1, arg2, result);
                            break;
                        }
                        case '/': {
                            printf("Found /\n");
                            tar++;
                            k = 1;

                            if (*tar == '-') {
                                printf("Found negative mult\n");
                                k = -1;
                                tar++;
                                printf("Current target char %c\n", *tar);
                            }
                            if ((cnt = getNumber(tar, &arg2)) <= 0) {
                                printf("No second argument\n");
                                strcpy(outMsg, "No second argument\n");
                                *tar = '\0';
                                break;
                            }

                            tar += cnt;
                            printf("Total arg2 = %f\n", arg2);
                            if (arg2 == 0 || arg2 == -0) {
                                strcpy(outMsg, "ERROR: divide by 0\n");
                                break;
                            }
                            arg2 *= k;
                            result = arg1 / arg2;
                            printf("Result : %f / %f = %f\n", arg1, arg2, result);
                            snprintf(outMsg, SIZE_MSG, "Result : %.2f / %.2f = %.2f\n", arg1, arg2, result);
                            break;
                        }
                        default: {
                            printf("Unknown operation\n");
                            strcpy(outMsg, "Unknown operation\n");
                            break;
                        }

                    }

                    send(sock, outMsg, sizeof(outMsg), 0);
                }


            }

        }
        memset(msg, 0, sizeof(msg));
        memset(outMsg, 0, sizeof(outMsg));
    }

    printf("Client %d left.\n", index);
    fflush(stdout);
}

u_int64_t factorial(int n) {
    u_int64_t r;
    for (r = 1; n > 1; r *= (n--));
    return r;
}

int getNumber(char *tar, double *res) {
    int count = 0;
    int num;
    while ('0' <= *tar && *tar <= '9') {
        num = *tar - '0';
//        printf("found digit %d\n", num);
        *res = *res * 10 + num;
//        printf("current res = %d\n\n", *res);
        count++;
        tar++;
    }
    return count;
}


void *connectionListener(void *args) {
    int listener = *((int *) args);

    int s;
    int indexClient;
    struct sockaddr_in a;
    int aLen = sizeof(a);
    for (;;) {

        s = accept(listener, (struct sockaddr *) &a, &aLen);
        if (s <= 0) {
            printf("STOPPING SERVER...\n");
            fflush(stdout);

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < clientQuantity; i++) {
                close(clients[i].socket);
                clients[i].socket = -1;
            }
            for (int i = 0; i < clientQuantity; i++) {
                pthread_join(clients[i].threadId, NULL);
            }
            pthread_mutex_unlock(&mutex);

            break;
        }

        pthread_mutex_lock(&mutex);
        clients = (struct tInfo *) realloc(clients, sizeof(struct tInfo) * (clientQuantity + 1));
        clients[clientQuantity].socket = s;
        clients[clientQuantity].address = inet_ntoa(a.sin_addr);
        clients[clientQuantity].port = a.sin_port;
        clients[clientQuantity].number = clientQuantity;

        indexClient = clientQuantity;
        if (pthread_create(&(clients[clientQuantity].threadId), NULL, clientHandler, (void *) &indexClient)) {
            printf("ERROR: Can't create thread for client!\n");
            fflush(stdout);
            continue;
        }

        pthread_mutex_unlock(&mutex);

        clientQuantity++;
    }

    printf("ENDED LISTENER CONNECTIONS!\n");
    fflush(stdout);
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

void *kickClient(int kickNum) {
    pthread_mutex_lock(&mutex);
    int check = 0;
    char msg[15] = "slish plati";
    for (int i = 0; i < clientQuantity; i++) {
        if (clients[i].number == kickNum) {
            shutdown(clients[i].socket, 2);
            close(clients[i].socket);
            clients[i].socket = -1;
            check = 1;
            break;
        }
    }
    if (check == 0) {
        printf("Client №%d not found, please try again.\n", kickNum);
    }
    pthread_mutex_unlock(&mutex);
}
