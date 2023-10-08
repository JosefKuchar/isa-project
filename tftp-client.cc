#include <unistd.h>
#include <iostream>
#include <memory>
#include "arpa/inet.h"
#include "client-args.h"
#include "enums.h"
#include "packet-builder.h"
#include "packet.h"
#include "utils.h"

enum class State {
    StartSend,
    StartSendNoOptions,
    StartRecieve,
    StartRecieveNoOptions,
    InitTransfer,
    Send,
    Recieve,
    End,
};

int main(int argc, char* argv[]) {
    // Parse arguments
    ClientArgs args(argc, argv);

    // Turn off buffering so we can see stdout and stderr in sync
    setbuf(stdout, NULL);

    // Create client address
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(0);
    socklen_t client_len = sizeof(client_addr);

    int sock;
    size_t BLKSIZE = 1024;
    char buffer[BUFSIZE] = {0};
    std::unique_ptr<char[]> file_buf;
    State state = args.send ? State::StartSend : State::StartRecieve;
    PacketBuilder packetBuilder(buffer);
    Packet packet;

    struct timeval tv;
    tv.tv_sec = 1;

    std::cout << "Connecting to " << inet_ntoa(args.address.sin_addr) << ":"
              << ntohs(args.address.sin_port) << std::endl;

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

    int block_count = 1;
    while (state != State::End) {
        switch (state) {
            case State::StartSend: {
                // Create and send WRQ packet
                packetBuilder.createWRQ(args.dest_filepath, "octet");
                packetBuilder.addBlksizeOption(BLKSIZE);
                packetBuilder.addTimeoutOption(1);
                send(sock, packetBuilder, &client_addr, &args.address);

                // Recieve packet
                packet = recieve(sock, buffer, &args.address, &client_addr, &args.len, -1);

                if (std::holds_alternative<OACKPacket>(packet)) {
                    // Parse options
                    state = State::InitTransfer;
                } else if (std::holds_alternative<ACKPacket>(packet)) {
                    // Fall back to default block size
                    BLKSIZE = DEFAULT_BLOCK_SIZE;
                    state = State::InitTransfer;
                } else if (std::holds_alternative<ERRORPacket>(packet)) {
                    // Server responded with error, try again without options
                    state = State::StartSendNoOptions;
                } else {
                    // Unexpected packet
                    std::cout << "Unexpected packet" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case State::StartSendNoOptions: {
                // Create and send WRQ packet
                packetBuilder.createWRQ(args.dest_filepath, "octet");
                args.address.sin_port = args.port;
                send(sock, packetBuilder, &client_addr, &args.address);

                // Recieve packet
                packet = recieve(sock, packetBuilder, &args.address, &client_addr, &args.len, -1);

                if (std::holds_alternative<ACKPacket>(packet)) {
                    // Fall back to default block size
                    BLKSIZE = DEFAULT_BLOCK_SIZE;
                    state = State::InitTransfer;
                } else {
                    // Unexpected packet
                    std::cout << "Unexpected packet" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case State::StartRecieve: {
                // Create and send RRQ packet
                packetBuilder.createRRQ(args.dest_filepath, "octet");
                // packetBuilder.addBlksizeOption(BLKSIZE);
                // packetBuilder.addTimeoutOption(1);
                send(sock, packetBuilder, &client_addr, &args.address);

                // Recieve packet
                packet = recieve(sock, packetBuilder, &args.address, &client_addr, &args.len, -1);

                if (std::holds_alternative<OACKPacket>(packet)) {
                    // Parse options
                    state = State::InitTransfer;
                } else if (std::holds_alternative<DATAPacket>(packet)) {
                    // Fall back to default block size
                    BLKSIZE = 512;
                    state = State::InitTransfer;
                } else if (std::holds_alternative<ERRORPacket>(packet)) {
                    // Server responded with error, try again without options
                    state = State::StartRecieveNoOptions;
                } else {
                    // Unexpected packet
                    std::cout << "Unexpected packet" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case State::InitTransfer: {
                // Allocate file buffer
                file_buf = std::make_unique<char[]>(BLKSIZE);
                if (args.send) {
                    state = State::Send;
                } else {
                    state = State::Recieve;
                }
                break;
            }
            case State::Send: {
                // Read file
                size_t blen = fread(file_buf.get(), 1, BLKSIZE, args.input_file);
                std::cout << "Read " << blen << " bytes" << std::endl;
                if (blen == 0) {
                    state = State::End;
                    break;
                }

                // Create and send DATA packet
                packetBuilder.createDATA(block_count, file_buf.get(), blen);
                send(sock, packetBuilder, &client_addr, &args.address);

                // Recieve packet
                packet = recieve(sock, packetBuilder, &args.address, &client_addr, &args.len, 0);

                if (std::holds_alternative<ACKPacket>(packet)) {
                    ACKPacket ack = std::get<ACKPacket>(packet);
                    // Check block number
                    if (ack.block == block_count) {
                        block_count++;
                    }
                } else {
                    // Unexpected packet
                    std::cout << "Unexpected packet" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case State::Recieve: {
                // Create and send ACK packet
                packetBuilder.createACK(block_count);
                send(sock, packetBuilder, &client_addr, &args.address);
                block_count++;

                // Recieve packet
                packet = recieve(sock, packetBuilder, &args.address, &client_addr, &args.len, 0);

                if (std::holds_alternative<DATAPacket>(packet)) {
                    DATAPacket data = std::get<DATAPacket>(packet);
                    // Check block number
                    if (data.block == block_count) {
                        // Write to file
                        std::cout << "Writing " << data.len << " bytes" << std::endl;
                        fwrite(data.data, 1, data.len, args.input_file);
                    }

                    if (data.len < BLKSIZE) {
                        state = State::End;
                    }
                } else {
                    // Unexpected packet
                    std::cout << "Unexpected packet" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}
