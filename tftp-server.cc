#include <thread>
#include "packet.h"
#include "server-args.h"

const size_t BUFSIZE = 65535;

void client_handler(struct sockaddr_in client_addr, char buffer[BUFSIZE], ssize_t len) {
    int sock = 0;

    Packet packet = parsePacket(buffer, len);
    printPacket(packet, client_addr, client_addr);
}

int main(int argc, char* argv[]) {
    ServerArgs args(argc, argv);
    int sock = 0;
    int opt = 1;
    char buffer[1024] = {0};
    size_t len = 0;

    struct sockaddr_in client_addr;

    setbuf(stdout, NULL);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the address and port
    if (bind(sock, (struct sockaddr*)&args.address, sizeof(args.address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    while (true) {
        ssize_t n =
            recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&client_addr, (socklen_t*)&len);

        std::thread client_thread(client_handler, client_addr, buffer, n);
    }

    return 0;
}
