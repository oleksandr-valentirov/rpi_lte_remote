#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>

#include "config.h"

#include <wiringPi.h>


#define PWM0            26
#define PWM1            23
#define CONF_PATH       "/etc/servo_cam/config.cfg"
#define CAM_PROC        "camera.py"
#define CAM_PROC_PATH   "/etc/servo_cam/camera.py"

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

typedef struct rc_cam_pos {
    int8_t x;
    int8_t y;
} __attribute__((__packed__)) rc_cam_pos_t;

typedef struct rc_connect_cmd {
    uint32_t ip;
    uint16_t port;
} __attribute__((__packed__)) rc_connect_cmd_t;

typedef struct rc_start_video_cmd {
    uint16_t port;
} __attribute__((__packed__)) rc_start_video_cmd_t;

typedef struct rc_auth {
    uint8_t type;
    char name[16];
} __attribute__((__packed__)) rc_auth_t;


uint8_t is_exit = 0;

static void config_socket(struct sockaddr_in *addr_struct_ptr, int *sock, uint16_t port, uint32_t ip);
static void intHandler(int signal);
static void kill_child_video_proc(pid_t *pid);


int main(int argc, char const *argv[]) {
    int cmd_sock = 0, default_ip = 0;
    struct sockaddr_in cmd_server_addr;
    rc_conn_state_t cmd_server_state = RC_NO_CONN;
    uint8_t buffer[256];
    ssize_t bytes_received = 0;
    rc_auth_t me;
    rc_header_t *rc_header = (rc_header_t *)buffer;
    uint16_t default_port = 7777;
    cfg_t *config = config_parse(CONF_PATH);
    pid_t video_pid = 0;
    char port_str[6];

    if (config == NULL) {
        printf("Invalid config file\r\n");
        return 1;
    }

    /* config name for authentication */
    memset(&me, 0, sizeof(rc_auth_t));
    me.type = 1;
    memcpy(&(me.name), cfg_getstr(config, "name"), strlen(cfg_getstr(config, "name")));

    /* config default server addr */
    default_port = (uint16_t)cfg_getint(config, "port");
    if (inet_pton(AF_INET, cfg_getstr(config, "ip_addr"), &default_ip) <= 0) {
        perror("Invalid default address or address not supported");
        return 1;
    }

    /* config PWM */
    if (wiringPiSetup () == -1) {
        perror("PWM setup failed");
        return 1;
    }
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(384000);
    pinMode(X, PWM_OUTPUT);
    pinMode(Y, PWM_OUTPUT);

    /* reg a signal handler */
    signal(SIGINT, intHandler);

    while (!is_exit) {
        switch (cmd_server_state) {
        case RC_NO_CONN:
            /* create socket */
            if ((cmd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation failed");
                return 1;
            }

            /* config socket */
            config_socket(&cmd_server_addr, &cmd_sock, default_port, default_ip);

            /* wait for connection to the default server */
            printf("Attempting to connect to the default server\r\n");
            while (connect(cmd_sock, (struct sockaddr *)&cmd_server_addr, sizeof(cmd_server_addr)) < 0)
                usleep(1000000);
            printf("Connected default server\r\n");
            cmd_server_state = RC_CONN;
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

                if (cmd_server_state == RC_NO_CONN) {
                    close(cmd_sock);
                    kill_child_video_proc(&video_pid);

                }
            } else {
                /* process and response */
                if (rc_header->payload_len) {
                    bytes_received = recv(cmd_sock, buffer + sizeof(rc_header_t), rc_header->payload_len, 0);
                    if (bytes_received < rc_header->payload_len) {
                        printf("Error: cmd class %d id %d payload is too short\r\n", rc_header->cmd_class, rc_header->cmd_id);
                        break;
                    }
                }

                if (rc_header->cmd_class == 2 && rc_header->cmd_id == 1) {
                    /* process a cmd to connect to another server */
                    close(cmd_sock);
                    /* create socket */
                    if ((cmd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        perror("Socket re-creation failed");
                        return 1;
                    }
                    config_socket(&cmd_server_addr, &cmd_sock, 
                        ((rc_connect_cmd_t *)(buffer + sizeof(rc_header_t)))->port, 
                        ((rc_connect_cmd_t *)(buffer + sizeof(rc_header_t)))->ip
                    );

                    printf("Attempting to connect to the new server...");                    
                    if (connect(cmd_sock, (struct sockaddr *)&cmd_server_addr, sizeof(cmd_server_addr)) < 0) {
                        close(cmd_sock);
                        cmd_server_state = RC_NO_CONN;
                        printf("failed\r\n");
                    } else {printf("success\r\n");}
                } else if (rc_header->cmd_class == 2 && rc_header->cmd_id == 3) {
                    /* disconnect */
                    close(cmd_sock);
                    cmd_server_state = RC_NO_CONN;
                    kill_child_video_proc(&video_pid);
                    printf("disconnected by cmd\r\n");
                    break;
                } else if (rc_header->cmd_class == 2 && rc_header->cmd_id == 4) {
                    /* start video stream */
                    if (video_pid != 0) {
                        printf("video already started\r\n");
                        break;
                    }

                    video_pid = fork();
                    if (video_pid < 0)
                        perror("video fork failed");
                    else if (video_pid == 0) {
                        printf("new process started...\r\n");
                        sprintf(port_str, "%u", ((rc_start_video_cmd_t *)(buffer + sizeof(rc_header_t)))->port);
                        execl(CAM_PROC_PATH, CAM_PROC, 
                            "--ip", inet_ntoa(cmd_server_addr.sin_addr), 
                            "--port", port_str,
                            (char *)NULL
                        );
                        printf("new process failed to start\r\n");
                    }
                } else if (rc_header->cmd_class == 3 && rc_header->cmd_id == 1) {
                    pwmWrite(X, 512 + 512 / 50 * (((rc_cam_pos_t *)(buffer + sizeof(rc_header_t)))->x));
                    pwmWrite(Y, 512 + 512 / 50 * (((rc_cam_pos_t *)(buffer + sizeof(rc_header_t)))->y));
                }
            }

            break;
        }
    }

    close(cmd_sock);

    return 0;
}

static void config_socket(struct sockaddr_in *addr_struct_ptr, int *sock, uint16_t port, uint32_t ip) {
    struct timeval tv;  /* timeout for the socket */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    memset(addr_struct_ptr, 0, sizeof(struct sockaddr_in));
    addr_struct_ptr->sin_family = AF_INET;
    addr_struct_ptr->sin_port = htons(port);
    addr_struct_ptr->sin_addr.s_addr = ip;
    setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

static void intHandler(int signal) {
    if (signal == SIGINT)
        is_exit = 1;
}

static void kill_child_video_proc(pid_t *pid) {
    if (!pid)
        return;
    kill(*pid, SIGINT);
    wait(NULL);
    *pid = 0;
}
