#include <iostream>
#include <csignal>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>


uint8_t is_exit = 0;

static void intHandler(int signal);


int main(int argc, char const *argv[]) {
    uint32_t video_dst_ip = 0;
    uint16_t video_dst_port = 0;
    struct sockaddr_in addr;
    int sock = 0;
    
    if (argc < 3)
        return 1;

    video_dst_port = static_cast<uint16_t>(std::stoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &video_dst_ip) <= 0) {
        std::cerr << "Invalid default address or address not supported" << std::endl;
        return 1;
    }

    /* config socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(video_dst_port);
    addr.sin_addr.s_addr = video_dst_ip;
    (void)sock;

    /* reg a signal handler */
    signal(SIGINT, intHandler);
    
    while (!is_exit) {

    }

    return 0;
}

static void intHandler(int signal) {
    if (signal == SIGINT)
        is_exit = 1;
}
