#include "utils.h"
#include <iostream>

void send(int sock,
          PacketBuilder builder,
          struct sockaddr_in* source_addr,
          struct sockaddr_in* dest_addr) {
    if (rand() % 3 == 0) {
        std::cout << "Packet lost: ";
    } else {
        sendto(sock, builder.getBuffer(), builder.getSize(), 0, (const struct sockaddr*)dest_addr,
               sizeof(*dest_addr));
    }
    Packet packet = parsePacket(builder.getBuffer(), builder.getSize());
    printPacket(packet, *source_addr, *dest_addr, true);
}

Packet recieve(int sock,
               PacketBuilder builder,
               struct sockaddr_in* source_addr,
               struct sockaddr_in* dest_addr,
               socklen_t* len,
               int resendDepth) {
    ssize_t n = recvfrom(sock, builder.getBuffer(), BUFSIZE, 0, (struct sockaddr*)source_addr, len);
    if (n < 0) {
        // Resend packet
        if (resendDepth >= 0 && resendDepth < MAX_RESEND_DEPTH) {
            send(sock, builder, dest_addr, source_addr);
            std::cout << "Packet lost, resending... " << resendDepth + 1 << "/" << MAX_RESEND_DEPTH
                      << std::endl;
        } else {
            throw TimeoutException();
        }

        return recieve(sock, builder, source_addr, dest_addr, len, resendDepth + 1);
    }
    Packet packet = parsePacket(builder.getBuffer(), n);
    printPacket(packet, *source_addr, *dest_addr, false);
    return packet;
}
