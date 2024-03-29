/**
 * @author Josef Kuchař (xkucha28)
 */

#pragma once

#include <arpa/inet.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "enums.h"

class InvalidStringException : public std::exception {
   public:
    const char* what() const throw() { return "String is invalid"; }
};

struct Options {
    std::optional<int> blkSize;
    std::optional<int> timeout;
    std::optional<long> tSize;
    bool valid;
};

struct RRQPacket {
    std::string filepath;
    std::string mode;
    std::vector<std::pair<std::string, std::string>> options;
};

struct WRQPacket {
    std::string filepath;
    std::string mode;
    std::vector<std::pair<std::string, std::string>> options;
};

struct DATAPacket {
    uint16_t block;
    char* data;
    size_t len;
};

struct ACKPacket {
    uint16_t block;
};

struct ERRORPacket {
    ErrorCode code;
    std::string message;
};

struct OACKPacket {
    std::vector<std::pair<std::string, std::string>> options;
};

struct UnknownPacket {};

typedef std::
    variant<RRQPacket, WRQPacket, DATAPacket, ACKPacket, ERRORPacket, OACKPacket, UnknownPacket>
        Packet;

/**
 * Parse packet from buffer
 * @param buffer Buffer
 * @param len Length of buffer
 */
Packet parsePacket(char* buffer, size_t len);

/**
 * Print packet to stderr
 * @param packet Packet
 * @param source Source address
 * @param dest Destination address
 */
void printPacket(Packet packet, sockaddr_in source, sockaddr_in dest, bool debug);

/**
 * Parse options vector to struct
 * @param opts Options vector
 * @return Options struct
 */
std::optional<Options> parseOptionsToStruct(std::vector<std::pair<std::string, std::string>> opts);
