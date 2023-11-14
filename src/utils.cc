/**
 * @author Josef Kuchař (xkucha28)
 */

#include "utils.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

void send(int sock,
          PacketBuilder builder,
          struct sockaddr_in* source_addr,
          struct sockaddr_in* dest_addr) {
#ifdef PACKET_LOSS
    if (rand() % 3 == 0) {
        std::cout << "Packet lost: ";
    } else {
#endif
        // sleep(1);
        sendto(sock, builder.getBuffer(), builder.getSize(), 0, (const struct sockaddr*)dest_addr,
               sizeof(*dest_addr));
#ifdef PACKET_LOSS
    }
#endif
    Packet packet = parsePacket(builder.getBuffer(), builder.getSize());
    printPacket(packet, *source_addr, *dest_addr, true);
}

Packet recieve(int sock,
               PacketBuilder builder,
               struct sockaddr_in* source_addr,
               struct sockaddr_in* dest_addr,
               socklen_t* len,
               std::atomic<bool>& running,
               int depth,
               bool saveAddr) {
    if (!running.load()) {
        throw InterruptException();
    }

    struct sockaddr_in currentSourceAddr;
    ssize_t n =
        recvfrom(sock, builder.getBuffer(), BUFSIZE, 0, (struct sockaddr*)&currentSourceAddr, len);
    if (n < 0) {
        if (!running.load()) {
            throw InterruptException();
        }
        if (depth >= 0 && depth < RETRY_COUNT) {
            std::cout << "Timeout, resending last packet (below)" << std::endl;
            send(sock, builder, dest_addr, source_addr);
            return recieve(sock, builder, source_addr, dest_addr, len, running, depth + 1);
        } else {
            throw TimeoutException();
        }
    }

    Packet packet = parsePacket(builder.getBuffer(), n);
    printPacket(packet, *source_addr, *dest_addr, false);

    if (saveAddr) {
        *source_addr = currentSourceAddr;
    } else {
        // Check if packet is from correct address
        if (currentSourceAddr.sin_addr.s_addr != source_addr->sin_addr.s_addr ||
            currentSourceAddr.sin_port != source_addr->sin_port) {
            std::cout << "Packet from wrong address, sending error to source" << std::endl;
            builder.createERROR(ErrorCode::UnknownTID, "Packet from wrong address");
            send(sock, builder, dest_addr, &currentSourceAddr);

            return recieve(sock, builder, source_addr, dest_addr, len, running, depth);
        }
    }

    return packet;
}

std::tuple<bool, bool, std::vector<char>> netasciiToBinary(char* buffer,
                                                           size_t size,
                                                           bool lastWasR) {
    std::vector<char> result;
    bool lastIsR = buffer[size - 1] == '\r';
    bool removeLast = false;

    for (size_t i = 0; i < size; i++) {
        if (i == 0 && lastWasR) {
            if (buffer[i] == '\n') {
                result.push_back('\n');
                removeLast = true;
            } else if (buffer[i] == '\0') {
                result.push_back('\r');
                removeLast = true;
            } else {
                result.push_back(buffer[i]);
            }
        } else if (i + 1 < size) {
            if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
                result.push_back('\n');
                i++;
            } else if (buffer[i] == '\r' && buffer[i] == '\0') {
                result.push_back('\r');
                i++;
            } else {
                result.push_back(buffer[i]);
            }
        } else {
            result.push_back(buffer[i]);
        }
    }
    return std::make_tuple(lastIsR, removeLast, result);
}

void binaryToNetascii(char* buffer, size_t size, std::vector<char>& netasciiBuffer) {
    for (size_t i = 0; i < size; i++) {
        if (buffer[i] == '\n') {
            netasciiBuffer.push_back('\r');
            netasciiBuffer.push_back('\n');
        } else if (buffer[i] == '\r') {
            netasciiBuffer.push_back('\r');
            netasciiBuffer.push_back('\0');
        } else {
            netasciiBuffer.push_back(buffer[i]);
        }
    }
}

int getFilesize(std::fstream& file, bool netascii) {
    // Loop through file to get size
    int size = 0;
    while (true) {
        char c;
        file.read(&c, 1);
        if (file.eof()) {
            break;
        }
        if (netascii && (c == '\n' || c == '\r')) {
            size++;
        }
        size++;
    }

    // Seek back to beginning
    file.clear();
    file.seekg(0, std::ios::beg);

    return size;
}
