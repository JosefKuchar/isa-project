#include <fstream>
#include <iostream>
#include <thread>
#include "packet-builder.h"
#include "packet.h"
#include "server-args.h"

void client_handler(struct sockaddr_in client_addr, char buffer[BUFSIZE], ssize_t len) {
    int sock = 0, opt = 1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(0);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    socklen_t server_len = sizeof(server_addr);

    PacketBuilder packetBuilder(buffer);
    Packet packet = parsePacket(buffer, len);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (getsockname(sock, (struct sockaddr*)&server_addr, &server_len)) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    printPacket(packet, client_addr, server_addr);

    std::ofstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    WRQPacket p = std::get<WRQPacket>(packet);
    try {
        file.open(p.filepath, std::ios::out | std::ios::binary);
    } catch (std::exception& e) {
        packetBuilder.createERROR(ErrorCode::FileNotFound, e.what());
        packet = parsePacket(buffer, packetBuilder.getSize());
        printPacket(packet, server_addr, client_addr);
        sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&client_addr,
               sizeof(client_addr));
    }

    packetBuilder.createACK(0);
    packet = parsePacket(buffer, packetBuilder.getSize());
    printPacket(packet, server_addr, client_addr);

    sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&client_addr,
           sizeof(client_addr));

    for (int i = 1;; i++) {
        ssize_t n = recvfrom(sock, (char*)buffer, BUFSIZE, 0, (struct sockaddr*)&client_addr,
                             (socklen_t*)&len);
        packet = parsePacket(buffer, n);
        printPacket(packet, client_addr, server_addr);

        DATAPacket data = std::get<DATAPacket>(packet);
        file.write(data.data, data.len);

        packetBuilder.createACK(i);
        packet = parsePacket(buffer, packetBuilder.getSize());
        printPacket(packet, server_addr, client_addr);

        sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&client_addr,
               sizeof(client_addr));

        if (data.len < 512) {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    ServerArgs args(argc, argv);
    int sock = 0;
    int opt = 1;
    char buffer[1024] = {0};
    size_t len = sizeof(args.address);

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

    std::cout << "Server listening on port " << ntohs(args.address.sin_port) << std::endl;

    while (true) {
        ssize_t n =
            recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&client_addr, (socklen_t*)&len);
        std::cout << "Received packet on main thread, creating new thread..." << std::endl;
        std::thread client_thread(client_handler, client_addr, buffer, n);
        client_thread.detach();
    }

    return 0;
}
