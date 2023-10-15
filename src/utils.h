#pragma once

#include "packet-builder.h"
#include "packet.h"

class TimeoutException : public std::exception {
   public:
    const char* what() const throw() { return "Timeout"; }
};

/**
 * Send packet
 * @param sock Socket
 * @param builder Packet builder with created packet
 * @param source_addr Source address (client)
 * @param dest_addr Destination address (server)
 */
void send(int sock,
          PacketBuilder builder,
          struct sockaddr_in* source_addr,
          struct sockaddr_in* dest_addr);

/**
 * Recieve packet
 * @param sock Socket
 * @param builder Packet builder
 * @param source_addr Source address (server)
 * @param dest_addr Destination address (client)
 * @param len Length of source address
 * @param depth Resend depth, -1 for no resend
 */
Packet recieve(int sock,
               PacketBuilder builder,
               struct sockaddr_in* source_addr,
               struct sockaddr_in* dest_addr,
               socklen_t* len,
               int depth = 0);

/**
 * Convert netascii to binary inplace
 * @param buffer Buffer
 * @param size Size of buffer
 * @return New size
 */
size_t netasciiToBinary(char* buffer, size_t size);

/**
 * Convert binary to netascii inplace (clips if too long)
 * @param buffer Buffer
 * @param size Size of data
 * @param maxSize Maximum size of buffer
 * @return How many bytes were clipped
 */
size_t binaryToNetascii(char* buffer, size_t size, size_t maxSize);
