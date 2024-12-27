#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <wiringPi.h>


#define PWM0        26
#define PWM1        23
#define PORT        8080
#define CONF_PATH   "/etc/servo_cam/config"

#define X   PWM0
#define Y   PWM1


uint8_t is_exit = 0;


int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in server_address;

    if (wiringPiSetup () == -1)
        exit(1);

    pinMode(X, PWM_OUTPUT);
    pinMode(Y, PWM_OUTPUT);

    /* Створення сокетум */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    /* Перетворення IP-адреси з текстового формату в бінарний */
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        exit(1);
    }

    /* Підключення до сервера */
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    while (!is_exit) {

    }

    return 0;
}
