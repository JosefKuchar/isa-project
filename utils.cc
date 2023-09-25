#include "utils.h"

void send(int sock,
          PacketBuilder builder,
          struct sockaddr_in* source_addr,
          struct sockaddr_in* dest_addr) {
    ssize_t n = sendto(sock, builder.getBuffer(), builder.getSize(), 0,
                       (const struct sockaddr*)dest_addr, sizeof(*dest_addr));
    Packet packet = parsePacket(builder.getBuffer(), builder.getSize());
    printPacket(packet, *source_addr, *dest_addr);
}

Packet recieve(int sock,
               char* buffer,
               struct sockaddr_in* source_addr,
               struct sockaddr_in* dest_addr,
               socklen_t* len) {
    ssize_t n = recvfrom(sock, (char*)buffer, BUFSIZE, 0, (struct sockaddr*)source_addr, len);
    if (n < 0) {
        throw TimeoutException();
    }
    Packet packet = parsePacket(buffer, n);
    printPacket(packet, *source_addr, *dest_addr);
    return packet;
}
