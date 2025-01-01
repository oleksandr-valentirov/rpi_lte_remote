#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <wiringPi.h>


#define PWM0        26
#define PWM1        23
#define CONF_PATH   "/etc/servo_cam/config"

#define X   PWM0
#define Y   PWM1

typedef enum rc_conn_state {
    RC_NO_CONN = 0,
    RC_CONN
} rc_conn_state_t;

typedef struct rc_header {
    uint8_t cmd_class;
    uint8_t cmd_id;
    uint16_t payload_len;
} __attribute__((__packed__)) rc_header_t;

typedef struct rc_connect_cmd {

} __attribute__((__packed__)) rc_connect_cmd_t;

typedef struct rc_cam_pos_cmd {
    uint8_t x;
    uint8_t y;
} __attribute__((__packed__)) rc_cam_pos_cmd_t;

typedef struct rc_auth {
    uint8_t type;
    char name[16];
} __attribute__((__packed__)) rc_auth_t;


uint8_t is_exit = 0;


int main(int argc, char const *argv[]) {
    int cmd_sock = 0;
    struct sockaddr_in cmd_server_addr;
    rc_conn_state_t cmd_server_state = RC_NO_CONN;
    uint8_t buffer[256];
    ssize_t bytes_received = 0;
    rc_auth_t me = {.type = 1, .name = "alpha"};
    rc_header_t *rc_header = (rc_header_t *)buffer;
    uint16_t cur_port = 0;

    /* timeout for the socket */
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;


    if (wiringPiSetup () == -1) {
        perror("PWM setup failed");
        return 1;
    }

    pinMode(X, PWM_OUTPUT);
    pinMode(Y, PWM_OUTPUT);

    while (!is_exit) {
        switch (cmd_server_state) {
        case RC_NO_CONN:
            /* create socket */
            if ((cmd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation failed");
                return 1;
            }

            /* config socket */
            memset(&cmd_server_addr, 0, sizeof(cmd_server_addr));
            cmd_server_addr.sin_family = AF_INET;
            cmd_server_addr.sin_port = htons(7777);
            setsockopt(cmd_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            if (inet_pton(AF_INET, "192.168.88.187", &cmd_server_addr.sin_addr) <= 0) {
                perror("Invalid default address or address not supported");
                close(cmd_sock);
                return 1;
            }

            /* wait for connection to the default server */
            printf("Attempting to connect to the default server\r\n");
            while (connect(cmd_sock, (struct sockaddr *)&cmd_server_addr, sizeof(cmd_server_addr)) < 0)
                usleep(1000000);
            printf("Connected default server\r\n");
            cmd_server_state = RC_CONN;
            cur_port = 7777;
            /* send auth packet and go straight to the next case */
            rc_header->cmd_class = 1;
            rc_header->cmd_id = 1;
            rc_header->payload_len = sizeof(rc_auth_t);
            memcpy(buffer + sizeof(rc_header_t), &me, sizeof(rc_auth_t));
            (void)write(cmd_sock, buffer, sizeof(rc_header_t) + sizeof(rc_auth_t));

        case RC_CONN:
            bytes_received = recv(cmd_sock, buffer, sizeof(rc_header_t), 0);

            if (bytes_received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                /* check if alive */
            } else if (bytes_received == 0) {
                cmd_server_state = RC_NO_CONN;
                /* try to reconnect */
                for (uint8_t i = 0; i < 3; i++) {
                    close(cmd_sock);

                    /* create socket */
                    if ((cmd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("Socket re-creation failed");
                        return 1;
                    }

                    /* config socket */
                    memset(&cmd_server_addr, 0, sizeof(cmd_server_addr));
                    cmd_server_addr.sin_family = AF_INET;
                    cmd_server_addr.sin_port = htons(cur_port);
                    if (inet_pton(AF_INET, "192.168.88.187", &cmd_server_addr.sin_addr) <= 0) {
                        perror("Invalid default address on re-creation or address not supported");
                        close(cmd_sock);
                        return 1;
                    }

                    printf("re-connecting\r\n");
                    if (connect(cmd_sock, (struct sockaddr *)&cmd_server_addr, sizeof(cmd_server_addr)) == 0) {
                        cmd_server_state = RC_CONN;
                        /* send auth packet */
                        rc_header->cmd_class = 1;
                        rc_header->cmd_id = 1;
                        rc_header->payload_len = sizeof(rc_auth_t);
                        memcpy(buffer + sizeof(rc_header_t), &me, sizeof(rc_auth_t));
                        (void)write(cmd_sock, buffer, sizeof(rc_header_t) + sizeof(rc_auth_t));
                        break;
                    }

                    usleep(1000000);
                }

                if (cmd_server_state == RC_NO_CONN)
                    close(cmd_sock);
            } else {
                /* process response */
            }

            break;
        }
    }

    close(cmd_sock);

    return 0;
}
