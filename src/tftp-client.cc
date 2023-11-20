/**
 * @author Josef Kuchař (xkucha28)
 */

#include <unistd.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <limits>
#include <memory>
#include "arpa/inet.h"
#include "client-args.h"
#include "enums.h"
#include "packet-builder.h"
#include "packet.h"
#include "utils.h"

enum class State {
    StartSend,
    StartRecieve,
    Send,
    Recieve,
    End,
};

std::atomic<bool> running(true);

/**
 * Handle transfer
 * @param sock Socket
 * @param clientAddr Client address
 * @param args Client arguments
 * @param tv Timeout
 */
bool handleTransfer(int sock, struct sockaddr_in* clientAddr, ClientArgs& args) {
    State state = args.send ? State::StartSend : State::StartRecieve;
    int blkSize = DEFAULT_BLOCK_SIZE;
    char buffer[BUFSIZE] = {0};
    char file_buf[BUFSIZE] = {0};
    PacketBuilder packetBuilder(buffer);
    Packet packet;
    int block_count = 1;
    bool next_block = true;
    size_t blen = 0;
    bool success = false;
    bool connected = false;

    try {
        while (state != State::End) {
            switch (state) {
                case State::StartSend: {
                    // Create and send WRQ packet
                    packetBuilder.createWRQ(args.dest_filepath, "octet");
                    send(sock, packetBuilder, clientAddr, &args.address);
                    // Recieve packet
                    packet = recieve(sock, packetBuilder, &args.address, clientAddr, &args.len,
                                     running, 0, true);
                    connected = true;
                    if (std::holds_alternative<ACKPacket>(packet)) {
                        state = State::Send;
                    } else {
                        // Unexpected packet
                        packetBuilder.createERROR(ErrorCode::IllegalOperation, "Unexpected packet");
                        send(sock, packetBuilder, clientAddr, &args.address);
                        state = State::End;
                        break;
                    }
                    break;
                }
                case State::StartRecieve: {
                    // Create and send RRQ packet
                    packetBuilder.createRRQ(args.dest_filepath, "octet");
                    send(sock, packetBuilder, clientAddr, &args.address);

                    // Recieve packet
                    packet = recieve(sock, packetBuilder, &args.address, clientAddr, &args.len,
                                     running, 0, true);
                    connected = true;

                    if (std::holds_alternative<DATAPacket>(packet)) {
                        state = State::Recieve;
                    } else {
                        // Unexpected packet
                        packetBuilder.createERROR(ErrorCode::IllegalOperation, "Unexpected packet");
                        send(sock, packetBuilder, clientAddr, &args.address);
                        state = State::End;
                        break;
                    }
                    break;
                }
                case State::Send: {
                    if (next_block) {
                        // Read file
                        blen = fread(file_buf, 1, blkSize, args.input_file);
                        std::cout << "Read " << blen << " bytes" << std::endl;

                        if (block_count == std::numeric_limits<uint16_t>::max()) {
                            std::cout << "Block count overflow" << std::endl;
                            packetBuilder.createERROR(ErrorCode::NotDefined,
                                                      "Block count overflow");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            state = State::End;
                            break;
                        }
                    }

                    // Create and send DATA packet
                    packetBuilder.createDATA(block_count, file_buf, blen);
                    send(sock, packetBuilder, clientAddr, &args.address);

                    // Recieve packet
                    packet =
                        recieve(sock, packetBuilder, &args.address, clientAddr, &args.len, running);

                    if (std::holds_alternative<ACKPacket>(packet)) {
                        ACKPacket ack = std::get<ACKPacket>(packet);
                        // Check block number
                        if (ack.block == block_count) {
                            block_count++;
                            next_block = true;

                            if (blen < (size_t)blkSize) {
                                success = true;
                                state = State::End;
                                break;
                            }
                        } else if (ack.block > block_count) {
                            // Unexpected block number
                            packetBuilder.createERROR(ErrorCode::IllegalOperation,
                                                      "ACK block number is too high");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            state = State::End;
                            break;
                        } else {
                            // Old ACK packet, resend DATA packet
                            next_block = false;
                        }
                    } else {
                        // Unexpected packet
                        packetBuilder.createERROR(ErrorCode::IllegalOperation, "Unexpected packet");
                        send(sock, packetBuilder, clientAddr, &args.address);
                        state = State::End;
                        break;
                    }
                    break;
                }
                case State::Recieve: {
                    if (std::holds_alternative<DATAPacket>(packet)) {
                        DATAPacket data = std::get<DATAPacket>(packet);
                        if (data.len > (size_t)blkSize) {
                            packetBuilder.createERROR(ErrorCode::NotDefined,
                                                      "Packet size is larger than block size");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            state = State::End;
                        }

                        // Check block number (we are expecting the next block)
                        if (data.block == block_count) {
                            // Write to file
                            std::cout << "Writing " << data.len << " bytes" << std::endl;
                            fwrite(data.data, 1, data.len, args.input_file);

                            block_count++;
                        } else if (data.block > block_count) {
                            // Unexpected block number
                            packetBuilder.createERROR(ErrorCode::IllegalOperation,
                                                      "DATA block number is too high");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            state = State::End;
                            break;
                        }

                        // Create and send ACK packet
                        packetBuilder.createACK(block_count - 1);
                        send(sock, packetBuilder, clientAddr, &args.address);

                        if (data.len < (size_t)blkSize) {
                            success = true;
                            state = State::End;
                            break;
                        }
                    } else {
                        // Unexpected packet
                        packetBuilder.createERROR(ErrorCode::IllegalOperation, "Unexpected packet");
                        send(sock, packetBuilder, clientAddr, &args.address);
                        state = State::End;
                        break;
                    }

                    // Recieve packet
                    packet =
                        recieve(sock, packetBuilder, &args.address, clientAddr, &args.len, running);
                    break;
                }
                default:
                    break;
            }
        }
    } catch (TimeoutException& e) {
        std::cout << "Timeout" << std::endl;
    } catch (InterruptException& e) {
        std::cout << "Interrupted" << std::endl;
        // Only send error if transfer is in progress
        if (connected) {
            packetBuilder.createERROR(ErrorCode::NotDefined, "Interrupted by user");
            send(sock, packetBuilder, clientAddr, &args.address);
        }
    }

    // Close file
    fclose(args.input_file);
    if (!args.send && !success) {
        // Remove file if transfer was not successful
        std::cout << "Removing file after failed transfer" << std::endl;
        remove(args.input_filepath.c_str());
    }
    // Close socket
    close(sock);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
    // Initalize random seed for packet loss simulation if enabled
    srand(time(NULL));

    // Parse arguments
    ClientArgs args(argc, argv);

    // Turn off buffering so we can see stdout and stderr in sync
    setbuf(stdout, NULL);

    std::signal(SIGINT, [](int) { running.store(false); });

    // Create client address
    struct sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(0);
    socklen_t clientLen = sizeof(clientAddr);

    int sock;

    struct timeval tv = {
        .tv_sec = DEFAULT_TIMEOUT,
        .tv_usec = 0,
    };

    std::cout << "Connecting to " << inet_ntoa(args.address.sin_addr) << ":"
              << ntohs(args.address.sin_port) << std::endl;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return EXIT_FAILURE;
    }
    // Bind socket to our address
    if (bind(sock, (const struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        std::cout << "Bind failed" << std::endl;
        return EXIT_FAILURE;
    }
    // Read assigned port
    if (getsockname(sock, (struct sockaddr*)&clientAddr, &clientLen)) {
        std::cout << "Reading binded port failed" << std::endl;
        return EXIT_FAILURE;
    }
    // Set timeout
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cout << "Set timeout failed" << std::endl;
        return EXIT_FAILURE;
    }

    // Handle transfer
    return handleTransfer(sock, &clientAddr, args);
}
