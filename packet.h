#pragma once

#include <arpa/inet.h>
#include <string>
#include <variant>
#include "enums.h"

struct RRQPacket {
    std::string filepath;
    std::string mode;
};

struct WRQPacket {
    std::string filepath;
    std::string mode;
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

struct OACKPacket {};

typedef std::variant<RRQPacket, WRQPacket, DATAPacket, ACKPacket, ERRORPacket, OACKPacket> Packet;

Packet parsePacket(char* buffer, size_t len);

void printPacket(Packet packet, sockaddr_in source, sockaddr_in dest);
