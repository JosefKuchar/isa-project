#include "utils.h"
#include <cstring>
#include <iostream>

void send(int sock,
          PacketBuilder builder,
          struct sockaddr_in* source_addr,
          struct sockaddr_in* dest_addr) {
#ifdef PACKET_LOSS
    if (rand() % 2 == 0) {
        std::cout << "Packet lost: ";
    } else {
#endif
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
               int depth) {
    ssize_t n = recvfrom(sock, builder.getBuffer(), BUFSIZE, 0, (struct sockaddr*)source_addr, len);
    if (n < 0) {
        if (depth >= 0 && depth < RETRY_COUNT) {
            std::cout << "Timeout, resending last packet (below)" << std::endl;
            send(sock, builder, dest_addr, source_addr);
            return recieve(sock, builder, source_addr, dest_addr, len, depth + 1);
        } else {
            throw TimeoutException();
        }
    }
    Packet packet = parsePacket(builder.getBuffer(), n);
    printPacket(packet, *source_addr, *dest_addr, false);
    return packet;
}

std::tuple<bool, bool, size_t> netasciiToBinary(char* buffer, size_t size, bool lastWasR) {
    bool lastR = buffer[size - 1] == '\r';
    bool removeLast = lastWasR && buffer[0] == '\n';
    size_t i = 0;
    while (i < size - 1) {
        if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
            buffer[i] = '\n';
            std::memmove(buffer + i + 1, buffer + i + 2, size - i - 2);
            size--;
        } else {
            i++;
        }
    }
    return std::make_tuple(lastR, removeLast, size);
}

size_t binaryToNetascii(char* buffer, size_t size, size_t maxSize) {
    size_t i = 0;
    size_t clipped = 0;
    while (i < size) {
        if (buffer[i] == '\n') {
            if (i + 1 >= maxSize) {
                clipped += size - i;
                size = i;
                break;
            }
            std::memmove(buffer + i + 2, buffer + i + 1, size - i - 1);
            buffer[i] = '\r';
            buffer[i + 1] = '\n';
            i += 2;
        } else {
            i++;
        }
    }
    return size - clipped;
}
