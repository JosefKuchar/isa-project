/**
 * @author Josef Kuchař (xkucha28)
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include "packet-builder.h"
#include "packet.h"
#include "server-args.h"
#include "utils.h"

enum class State { Start, StartRecieve, Recieve, Send, End };

void client_handler(struct sockaddr_in client_addr, Packet packet, std::filesystem::path path) {
    // Initalize random seed for packet loss simulation if enabled
    srand(time(NULL));
    // Turn off buffering so we can see stdout and stderr in sync
    setbuf(stdout, NULL);

    int sock = 0, opt = 1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(0);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    socklen_t len = sizeof(server_addr);
    struct timeval tv;
    tv.tv_usec = 999999;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        std::cout << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cout << "Failed to set socket options" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "Failed to bind socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (getsockname(sock, (struct sockaddr*)&server_addr, &len)) {
        std::cout << "Failed to get socket name" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cout << "Failed to set socket timeout" << std::endl;
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZE] = {0};
    char fileBuffer[BUFSIZE] = {0};
    PacketBuilder packetBuilder(buffer);
    State state = State::Start;
    std::fstream file;

    bool nextBlock = true;
    int bytesRead = 0;
    bool netascii = false;
    bool lastWasR = false;
    int currentBlock = 1;
    int blockSize = DEFAULT_BLOCK_SIZE;

    try {
        while (state != State::End) {
            switch (state) {
                case State::Start: {
                    // Read request
                    if (std::holds_alternative<RRQPacket>(packet)) {
                        RRQPacket rrq = std::get<RRQPacket>(packet);
                        path /= rrq.filepath;

                        file.open(path, std::ios::in | std::ios::binary);
                        if (!file.is_open()) {
                            packetBuilder.createERROR(ErrorCode::FileNotFound, "File not found");
                            send(sock, packetBuilder, &server_addr, &client_addr);
                            state = State::End;
                            break;
                        }

                        if (rrq.mode == "netascii") {
                            netascii = true;
                        } else if (rrq.mode == "octet") {
                            netascii = false;
                        } else {
                            packetBuilder.createERROR(ErrorCode::IllegalOperation, "Invalid mode");
                            send(sock, packetBuilder, &server_addr, &client_addr);
                            state = State::End;
                            break;
                        }

                        std::optional<Options> options = parseOptionsToStruct(rrq.options);
                        if (options.has_value()) {
                            if (!options->valid) {
                                packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                          "Invalid options");
                                send(sock, packetBuilder, &server_addr, &client_addr);
                                state = State::End;
                                break;
                            }

                            packetBuilder.createOACK();
                            if (options->blkSize.has_value()) {
                                blockSize = options->blkSize.value();
                                packetBuilder.addBlksizeOption(blockSize);
                            }
                            if (options->timeout.has_value()) {
                                packetBuilder.addTimeoutOption(options->timeout.value());
                                tv.tv_sec = options->timeout.value();
                                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
                                    0) {
                                    std::cout << "Failed to set socket timeout" << std::endl;
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (options->tSize.has_value()) {
                                if (options->tSize.value() != 0) {
                                    packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                              "Tsize should be 0 for RRQ");
                                    send(sock, packetBuilder, &server_addr, &client_addr);
                                    state = State::End;
                                    break;
                                }
                                int size = getFilesize(file, netascii);
                                std::cout << "Filesize: " << size << std::endl;
                                packetBuilder.addTsizeOption(size);
                            }

                            send(sock, packetBuilder, &server_addr, &client_addr);

                            Packet packet =
                                recieve(sock, packetBuilder, &client_addr, &server_addr, &len);
                            if (std::holds_alternative<ACKPacket>(packet)) {
                                ACKPacket ack = std::get<ACKPacket>(packet);
                                if (ack.block == 0) {
                                    std::cout << "Confirmed options" << std::endl;
                                } else {
                                    packetBuilder.createERROR(ErrorCode::NotDefined,
                                                              "Expected block 0");
                                    send(sock, packetBuilder, &server_addr, &client_addr);
                                    state = State::End;
                                    break;
                                }
                            } else {
                                packetBuilder.createERROR(ErrorCode::NotDefined,
                                                          "Expected ACK packet");
                                send(sock, packetBuilder, &server_addr, &client_addr);
                                state = State::End;
                                break;
                            }
                        }

                        state = State::Send;
                        // Write request
                    } else if (std::holds_alternative<WRQPacket>(packet)) {
                        WRQPacket wrq = std::get<WRQPacket>(packet);
                        path /= wrq.filepath;
                        if (std::filesystem::exists(path)) {
                            packetBuilder.createERROR(ErrorCode::FileAlreadyExists,
                                                      "File already exists");
                            send(sock, packetBuilder, &server_addr, &client_addr);
                            state = State::End;
                            break;
                        }
                        try {
                            file.open(path, std::ios::out | std::ios::binary);
                            if (!file.is_open()) {
                                throw std::runtime_error("Failed to open file");
                            }
                        } catch (std::exception& e) {
                            packetBuilder.createERROR(ErrorCode::FileNotFound, e.what());
                            send(sock, packetBuilder, &server_addr, &client_addr);
                            state = State::End;
                            break;
                        }

                        if (wrq.mode == "netascii") {
                            netascii = true;
                        } else if (wrq.mode == "octet") {
                            netascii = false;
                        } else {
                            packetBuilder.createERROR(ErrorCode::IllegalOperation, "Invalid mode");
                            send(sock, packetBuilder, &server_addr, &client_addr);
                            state = State::End;
                            break;
                        }

                        std::optional<Options> options = parseOptionsToStruct(wrq.options);
                        if (options.has_value()) {
                            if (!options->valid) {
                                packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                          "Invalid options");
                                send(sock, packetBuilder, &server_addr, &client_addr);
                                state = State::End;
                                break;
                            }

                            packetBuilder.createOACK();
                            if (options->blkSize.has_value()) {
                                blockSize = options->blkSize.value();
                                packetBuilder.addBlksizeOption(blockSize);
                            }
                            if (options->timeout.has_value()) {
                                packetBuilder.addTimeoutOption(options->timeout.value());
                                tv.tv_sec = options->timeout.value();
                                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
                                    0) {
                                    std::cout << "Failed to set socket timeout" << std::endl;
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (options->tSize.has_value()) {
                                packetBuilder.addTsizeOption(options->tSize.value());
                            }

                            send(sock, packetBuilder, &server_addr, &client_addr);
                        } else {
                            packetBuilder.createACK(0);
                            send(sock, packetBuilder, &server_addr, &client_addr);
                        }

                        state = State::Recieve;
                        // Invalid request
                    } else {
                        packetBuilder.createERROR(ErrorCode::IllegalOperation, "Illegal operation");
                        send(sock, packetBuilder, &server_addr, &client_addr);
                        state = State::End;
                    }
                    break;
                }
                case State::Recieve: {
                    Packet packet = recieve(sock, packetBuilder, &client_addr, &server_addr, &len);
                    if (std::holds_alternative<DATAPacket>(packet)) {
                        DATAPacket data = std::get<DATAPacket>(packet);
                        if (data.block == currentBlock) {
                            currentBlock++;

                            size_t len = data.len;
                            if (netascii) {
                                auto ret = netasciiToBinary(data.data, data.len, lastWasR);
                                lastWasR = std::get<0>(ret);
                                if (std::get<1>(ret)) {
                                    file.seekp(-1, std::ios::cur);
                                }
                                len = std::get<2>(ret);
                            }
                            file.write(data.data, len);
                        }

                        packetBuilder.createACK(currentBlock - 1);
                        send(sock, packetBuilder, &server_addr, &client_addr);
                        if (data.len < (size_t)blockSize) {
                            state = State::End;
                        }
                    } else {
                        packetBuilder.createERROR(ErrorCode::IllegalOperation,
                                                  "Expected DATA packet");
                        send(sock, packetBuilder, &server_addr, &client_addr);
                        state = State::End;
                    }
                    break;
                }
                case State::Send: {
                    if (nextBlock) {
                        file.read(fileBuffer, blockSize);
                        if (netascii) {
                            int size = binaryToNetascii(fileBuffer, file.gcount(), blockSize);
                            if (size < 0) {
                                file.seekg(size, std::ios::cur);
                                bytesRead = blockSize;
                            } else {
                                bytesRead = size;
                            }
                        } else {
                            bytesRead = file.gcount();
                        }
                    }
                    packetBuilder.createDATA(currentBlock, fileBuffer, bytesRead);
                    send(sock, packetBuilder, &server_addr, &client_addr);

                    Packet packet = recieve(sock, packetBuilder, &client_addr, &server_addr, &len);
                    if (std::holds_alternative<ACKPacket>(packet)) {
                        ACKPacket ack = std::get<ACKPacket>(packet);
                        if (ack.block == currentBlock) {
                            std::cout << "Confirmed block " << currentBlock << std::endl;
                            currentBlock++;
                            nextBlock = true;
                        } else {
                            nextBlock = false;
                            break;
                        }
                    } else {
                        packetBuilder.createERROR(ErrorCode::IllegalOperation,
                                                  "Expected ACK packet");
                        send(sock, packetBuilder, &server_addr, &client_addr);
                        break;
                    }

                    if (bytesRead < blockSize) {
                        state = State::End;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    } catch (TimeoutException& e) {
        std::cout << "Timeout, aborting" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Turn off buffering so we can see stdout and stderr in sync
    setbuf(stdout, NULL);

    ServerArgs args(argc, argv);
    int sock = 0;
    int opt = 1;
    char buffer[BUFSIZE] = {0};
    socklen_t len = sizeof(args.address);
    PacketBuilder packetBuilder(buffer);
    struct sockaddr_in client_addr;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        std::cout << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Attach socket to the port
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cout << "Failed to set socket options" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Bind socket to the address and port
    if (bind(sock, (struct sockaddr*)&args.address, sizeof(args.address)) < 0) {
        std::cout << "Failed to bind socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << ntohs(args.address.sin_port) << std::endl;

    // Main loop
    while (true) {
        Packet packet = recieve(sock, packetBuilder, &client_addr, &args.address, &len, -1, true);
        std::cout << "Received packet on main thread, creating new thread..." << std::endl;
        std::thread client_thread(client_handler, client_addr, packet, args.path);
        client_thread.detach();
    }
    return EXIT_SUCCESS;
}
