/**
 * @author Josef Kuchař (xkucha28)
 */

#pragma once

#include <atomic>
#include <fstream>
#include <tuple>
#include "packet-builder.h"
#include "packet.h"

class TimeoutException : public std::exception {
   public:
    const char* what() const throw() { return "Timeout"; }
};

class InterruptException : public std::exception {
   public:
    const char* what() const throw() { return "Interrupted"; }
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
 * @param saveAddr Whether to save address of sender
 */
Packet recieve(int sock,
               PacketBuilder builder,
               struct sockaddr_in* source_addr,
               struct sockaddr_in* dest_addr,
               socklen_t* len,
               std::atomic<bool>& running,
               int depth = 0,
               bool saveAddr = false);

/**
 * Convert netascii to binary and return as vector
 * @param buffer Buffer
 * @param size Size of buffer
 * @return Tuple of (If last character was \r, Whether to remove last byte from file, newSize)
 */
std::tuple<bool, bool, std::vector<char>> netasciiToBinary(char* buffer,
                                                           size_t size,
                                                           bool lastWasR);

/**
 * Convert binary to netascii - adds to netasciiBuffer vector
 * @param buffer Buffer with binary data
 * @param netasciiBuffer Buffer for netascii data
 */
void binaryToNetascii(char* buffer, size_t size, std::vector<char>& netasciiBuffer);

/**
 * Get filesize
 * @param file File
 * @param netascii Whether to we are using netascii
 * @return Filesize
 */
int getFilesize(std::fstream& file, bool netascii);
