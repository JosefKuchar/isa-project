/**
 * @author Josef Kuchař (xkucha28)
 */

#include <unistd.h>
#include <atomic>
#include <csignal>
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
    StartRecieve,
    InitTransfer,
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
void handleTransfer(int sock, struct sockaddr_in* clientAddr, ClientArgs& args, struct timeval tv) {
    State state = args.send ? State::StartSend : State::StartRecieve;
    int blkSize = 1024;
    char buffer[BUFSIZE] = {0};
    std::unique_ptr<char[]> file_buf;
    PacketBuilder packetBuilder(buffer);
    Packet packet;
    int block_count = 1;
    bool next_block = true;
    size_t blen = 0;

    try {
        while (state != State::End) {
            switch (state) {
                case State::StartSend: {
                    // Create and send WRQ packet
                    packetBuilder.createWRQ(args.dest_filepath, "octet");
                    packetBuilder.addBlksizeOption(blkSize);
                    packetBuilder.addTimeoutOption(DEFAULT_TIMEOUT);
                    send(sock, packetBuilder, clientAddr, &args.address);

                    // Recieve packet
                    packet = recieve(sock, packetBuilder, &args.address, clientAddr, &args.len,
                                     running, 0, true);

                    // Options
                    if (std::holds_alternative<OACKPacket>(packet)) {
                        OACKPacket oack = std::get<OACKPacket>(packet);
                        std::optional<Options> options = parseOptionsToStruct(oack.options);
                        if (options.has_value()) {
                            if (!options->valid) {
                                packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                          "Invalid option");
                                send(sock, packetBuilder, clientAddr, &args.address);
                                exit(EXIT_FAILURE);
                            }

                            // Parse options
                            if (options->blkSize.has_value()) {
                                if (options->blkSize.value() > blkSize) {
                                    packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                              "Invalid block size");
                                    send(sock, packetBuilder, clientAddr, &args.address);
                                    exit(EXIT_FAILURE);
                                }
                                blkSize = options->blkSize.value();
                            }
                            if (options->timeout.has_value()) {
                                if (options->timeout.value() != DEFAULT_TIMEOUT) {
                                    packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                              "Timeout does not match");
                                    send(sock, packetBuilder, clientAddr, &args.address);
                                    exit(EXIT_FAILURE);
                                }
                                tv.tv_sec = options->timeout.value();
                                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
                                    0) {
                                    std::cout << "Set timeout failed" << std::endl;
                                    exit(EXIT_FAILURE);
                                }
                            }

                            state = State::InitTransfer;
                        } else {
                            packetBuilder.createERROR(ErrorCode::InvalidOption, "Expected options");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            exit(EXIT_FAILURE);
                        }

                        state = State::InitTransfer;
                    } else if (std::holds_alternative<ACKPacket>(packet)) {
                        // Fall back to default block size
                        blkSize = DEFAULT_BLOCK_SIZE;
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
                    packetBuilder.addTsizeOption(0);
                    packetBuilder.addBlksizeOption(blkSize);
                    packetBuilder.addTimeoutOption(DEFAULT_TIMEOUT);
                    send(sock, packetBuilder, clientAddr, &args.address);

                    // Recieve packet
                    packet = recieve(sock, packetBuilder, &args.address, clientAddr, &args.len,
                                     running, 0, true);

                    // Options
                    if (std::holds_alternative<OACKPacket>(packet)) {
                        OACKPacket oack = std::get<OACKPacket>(packet);
                        std::optional<Options> options = parseOptionsToStruct(oack.options);
                        if (options.has_value()) {
                            if (!options->valid) {
                                packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                          "Invalid option");
                                send(sock, packetBuilder, clientAddr, &args.address);
                                exit(EXIT_FAILURE);
                            }

                            // Parse options
                            if (options->blkSize.has_value()) {
                                if (options->blkSize.value() > blkSize) {
                                    packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                              "Invalid block size");
                                    send(sock, packetBuilder, clientAddr, &args.address);
                                    exit(EXIT_FAILURE);
                                }
                                blkSize = options->blkSize.value();
                            }
                            if (options->timeout.has_value()) {
                                if (options->timeout.value() != DEFAULT_TIMEOUT) {
                                    packetBuilder.createERROR(ErrorCode::InvalidOption,
                                                              "Timeout does not match");
                                    send(sock, packetBuilder, clientAddr, &args.address);
                                    exit(EXIT_FAILURE);
                                }
                                tv.tv_sec = options->timeout.value();
                                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
                                    0) {
                                    std::cout << "Set timeout failed" << std::endl;
                                    exit(EXIT_FAILURE);
                                }
                            }

                            state = State::InitTransfer;
                        } else {
                            packetBuilder.createERROR(ErrorCode::InvalidOption, "Expected options");
                            send(sock, packetBuilder, clientAddr, &args.address);
                            exit(EXIT_FAILURE);
                        }

                        while (true) {
                            packetBuilder.createACK(0);
                            send(sock, packetBuilder, clientAddr, &args.address);

                            packet = recieve(sock, packetBuilder, &args.address, clientAddr,
                                             &args.len, running);
                            if (std::holds_alternative<DATAPacket>(packet)) {
                                state = State::InitTransfer;
                                break;
                            } else if (std::holds_alternative<OACKPacket>(packet)) {
                                // Old packet, resend ACK
                                std::cout << "Old packet, resending ACK" << std::endl;
                            } else {
                                // Unexpected packet
                                std::cout << "Unexpected packet" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                        }

                        state = State::InitTransfer;
                    } else if (std::holds_alternative<DATAPacket>(packet)) {
                        // Fall back to default block size
                        blkSize = 512;
                        state = State::InitTransfer;
                    } else {
                        // Unexpected packet
                        std::cout << "Unexpected packet" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                case State::InitTransfer: {
                    // Allocate file buffer
                    file_buf = std::make_unique<char[]>(blkSize);
                    if (args.send) {
                        state = State::Send;
                    } else {
                        state = State::Recieve;
                    }
                    break;
                }
                case State::Send: {
                    if (next_block) {
                        // Read file
                        blen = fread(file_buf.get(), 1, blkSize, args.input_file);
                        std::cout << "Read " << blen << " bytes" << std::endl;
                        if (blen == 0) {
                            state = State::End;
                            break;
                        }
                    }

                    // Create and send DATA packet
                    packetBuilder.createDATA(block_count, file_buf.get(), blen);
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
                        } else {
                            // Old ACK packet, resend DATA packet
                            next_block = false;
                        }
                    } else if (std::holds_alternative<OACKPacket>(packet)) {
                        // Old packet, resend DATA packet
                        next_block = false;
                    } else {
                        // Unexpected packet
                        std::cout << "Unexpected packet" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                case State::Recieve: {
                    if (std::holds_alternative<DATAPacket>(packet)) {
                        DATAPacket data = std::get<DATAPacket>(packet);
                        // Check block number (we are expecting the next block)
                        if (data.block == block_count) {
                            // Write to file
                            std::cout << "Writing " << data.len << " bytes" << std::endl;
                            fwrite(data.data, 1, data.len, args.input_file);

                            block_count++;
                        }
                        // Create and send ACK packet
                        packetBuilder.createACK(block_count - 1);
                        send(sock, packetBuilder, clientAddr, &args.address);

                        if (data.len < (size_t)blkSize) {
                            state = State::End;
                            break;
                        }
                    } else if (std::holds_alternative<OACKPacket>(packet)) {
                        // Old packet, resend ACK packet
                    } else {
                        // Unexpected packet
                        std::cout << "Unexpected packet" << std::endl;
                        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    } catch (InterruptException& e) {
        std::cout << "Interrupted" << std::endl;
        // TODO: Only send error if transfer is in progress
        packetBuilder.createERROR(ErrorCode::NotDefined, "Interrupted by user");
        send(sock, packetBuilder, clientAddr, &args.address);
        exit(EXIT_FAILURE);
    }
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
        exit(EXIT_FAILURE);
    }
    // Bind socket to our address
    if (bind(sock, (const struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        std::cout << "Bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Read assigned port
    if (getsockname(sock, (struct sockaddr*)&clientAddr, &clientLen)) {
        std::cout << "Reading binded port failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    // Set timeout
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cout << "Set timeout failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Handle transfer
    handleTransfer(sock, &clientAddr, args, tv);

    return EXIT_SUCCESS;
}
