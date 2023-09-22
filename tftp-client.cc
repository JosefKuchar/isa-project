#include <iostream>
#include "arpa/inet.h"
#include "client-args.h"
#include "enums.h"
#include "packet.h"
#include "utils.h"

int main(int argc, char* argv[]) {
    ClientArgs args(argc, argv);

    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);

    int sock;

    bind(sock, (const struct sockaddr*)&local_addr, sizeof(local_addr));

    // Find out what port was assigned
    struct sockaddr_in recv_address;
    getsockname(sock, (struct sockaddr*)&recv_address, (socklen_t*)sizeof(recv_address));
    std::cout << "Assigned port: " << ntohs(recv_address.sin_port) << std::endl;
    recv_address.sin_family = AF_INET;
    recv_address.sin_addr.s_addr = args.address.sin_addr.s_addr;

    int len = 0;
    char buffer[1024] = {0};
    std::string line;

    Packet packet(buffer);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation error." << std::endl;
        exit(EXIT_FAILURE);
    }

    packet.createWRQ(args.dest_filepath, "netascii");

    std::cout << "Sending" << std::endl;

    print_packet(buffer, recv_address);

    sendto(sock, buffer, packet.getSize(), 0, (const struct sockaddr*)&args.address,
           sizeof(args.address));

    std::cout << "Waiting" << std::endl;

    ssize_t n =
        recvfrom(sock, (char*)buffer, 1024, 0, (struct sockaddr*)&args.address, (socklen_t*)&len);

    print_packet(buffer, args.address);

    return 0;
}
