#include <unistd.h>
#include <iostream>
#include "arpa/inet.h"
#include "client-args.h"
#include "enums.h"
#include "packet-builder.h"
#include "packet.h"

const size_t BUFSIZE = 65535;

int main(int argc, char* argv[]) {
    // Parse arguments
    ClientArgs args(argc, argv);

    // Turn off buffering so we can see stdout and stderr in sync
    setbuf(stdout, NULL);

    std::cout << "Connecting to " << inet_ntoa(args.address.sin_addr) << ":"
              << ntohs(args.address.sin_port) << std::endl;

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);
    socklen_t client_len = sizeof(client_addr);

    struct timeval tv;
    tv.tv_sec = 1;

    int sock, len = sizeof(args.address);
    char buffer[BUFSIZE] = {0};
    std::string line;

    PacketBuilder packetBuilder(buffer);
    Packet packet;

    size_t BLKSIZE = 1024;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation error." << std::endl;
        exit(EXIT_FAILURE);
    }
    // Bind socket to our address
    if (bind(sock, (const struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    // Read assigned port
    if (getsockname(sock, (struct sockaddr*)&client_addr, &client_len)) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }
    // Set timeout
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    packetBuilder.createWRQ(args.dest_filepath, "octet");
    packetBuilder.addBlksizeOption(BLKSIZE);
    packetBuilder.addTimeoutOption(1);
    packet = parsePacket(buffer, packetBuilder.getSize());
    printPacket(packet, client_addr, args.address);

    sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&args.address,
           sizeof(args.address));

    ssize_t n = recvfrom(sock, (char*)buffer, BUFSIZE, 0, (struct sockaddr*)&args.address,
                         (socklen_t*)&len);
    if (n < 0) {
        std::cerr << "Timeout" << std::endl;
        exit(EXIT_FAILURE);
    }

    packet = parsePacket(buffer, n);
    printPacket(packet, args.address, client_addr);

    if (std::holds_alternative<ACKPacket>(packet)) {
        ACKPacket ack = std::get<ACKPacket>(packet);
        BLKSIZE = 512;
        std::cout << "Server responded with ACK, falling back to default block size of 512"
                  << std::endl;
    }

    char file_buf[BLKSIZE] = {0};
    size_t blen = 0;

    int block_count = 1;
    int bl = 1;
    bool good = true;

    while (true) {
        if (!good) {
            std::cout << "Block number does not match, resending..." << std::endl;
        } else {
            blen = fread(file_buf, 1, BLKSIZE, args.input_file);
            std::cout << "Read " << blen << " bytes" << std::endl;
            if (blen == 0) {
                break;
            }
        }

        packetBuilder.createDATA(block_count, file_buf, blen);
        packet = parsePacket(buffer, packetBuilder.getSize());
        printPacket(packet, client_addr, args.address);

        sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&args.address,
               sizeof(args.address));

        n = recvfrom(sock, (char*)buffer, BUFSIZE, 0, (struct sockaddr*)&args.address,
                     (socklen_t*)&len);

        packet = parsePacket(buffer, n);
        printPacket(packet, args.address, client_addr);

        bl = ntohs(*(short*)(buffer + 2));

        good = bl == block_count;
        if (good) {
            block_count++;
        }
    }

    return 0;
}
