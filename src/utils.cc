#include "utils.h"
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
               socklen_t* len) {
    ssize_t n = recvfrom(sock, builder.getBuffer(), BUFSIZE, 0, (struct sockaddr*)source_addr, len);
    if (n < 0) {
        throw TimeoutException();
    }
    Packet packet = parsePacket(builder.getBuffer(), n);
    printPacket(packet, *source_addr, *dest_addr, false);
    return packet;
}
