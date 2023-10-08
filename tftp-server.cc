#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include "packet-builder.h"
#include "packet.h"
#include "server-args.h"
#include "utils.h"

enum class State { Start, StartRecieve, Recieve, Send, End };

void client_handler(struct sockaddr_in client_addr, Packet packet, std::filesystem::path basePath) {
    int sock = 0, opt = 1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(0);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    socklen_t len = sizeof(server_addr);

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
    if (getsockname(sock, (struct sockaddr*)&server_addr, &len)) {
        perror("getsockname");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZE] = {0};
    PacketBuilder packetBuilder(buffer);
    State state = State::Start;
    int currentBlock = 1;
    int blockSize = DEFAULT_BLOCK_SIZE;
    std::fstream file;

    while (state != State::End) {
        switch (state) {
            case State::Start: {
                if (std::holds_alternative<RRQPacket>(packet)) {
                    RRQPacket rrq = std::get<RRQPacket>(packet);
                    if (rrq.options.size() > 0) {
                        // TODO: Parse options
                        packetBuilder.createOACK();
                    } else {
                        // TODO: Send first data packet
                    }
                } else if (std::holds_alternative<WRQPacket>(packet)) {
                    WRQPacket wrq = std::get<WRQPacket>(packet);
                    if (wrq.options.size() > 0) {
                        // TODO: Parse options
                        packetBuilder.createOACK();
                    } else {
                        packetBuilder.createACK(0);
                        send(sock, packetBuilder, &server_addr, &client_addr);
                    }

                    state = State::Recieve;
                } else {
                    packetBuilder.createERROR(ErrorCode::IllegalOperation, "Illegal operation");
                    send(sock, packetBuilder, &server_addr, &client_addr);
                    state = State::End;
                }
                break;
            }
            case State::Recieve: {
                // file.open("test.txt", std::ios::out | std::ios::binary | std::ios::);

                Packet packet = recieve(sock, buffer, &client_addr, &server_addr, &len);
                if (std::holds_alternative<DATAPacket>(packet)) {
                    DATAPacket data = std::get<DATAPacket>(packet);
                    file.write(data.data, data.len);
                    packetBuilder.createACK(currentBlock);
                    send(sock, packetBuilder, &server_addr, &client_addr);

                    currentBlock++;

                    if (data.len < blockSize) {
                        state = State::End;
                    }
                } else {
                    packetBuilder.createERROR(ErrorCode::IllegalOperation, "Expected DATA packet");
                }
                break;
            }
            case State::Send: {
                break;
            }
        }
    }

    // std::ofstream file;
    // file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    // WRQPacket p = std::get<WRQPacket>(packet);
    // try {
    //     file.open(p.filepath, std::ios::out | std::ios::binary);
    // } catch (std::exception& e) {
    //     packetBuilder.createERROR(ErrorCode::FileNotFound, e.what());
    //     packet = parsePacket(buffer, packetBuilder.getSize());
    //     printPacket(packet, server_addr, client_addr, true);
    //     sendto(sock, buffer, packetBuilder.getSize(), 0, (const struct sockaddr*)&client_addr,
    //            sizeof(client_addr));
    // }
}

int main(int argc, char* argv[]) {
    ServerArgs args(argc, argv);
    int sock = 0;
    int opt = 1;
    char buffer[BUFSIZE] = {0};
    socklen_t len = sizeof(args.address);

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
        Packet packet = recieve(sock, buffer, &client_addr, &args.address, &len);
        std::cout << "Received packet on main thread, creating new thread..." << std::endl;
        std::thread client_thread(client_handler, client_addr, packet, args.path);
        client_thread.detach();
    }

    return 0;
}
