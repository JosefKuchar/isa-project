#include "packet.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include "enums.h"

/**
 * Parse options from buffer
 * @param buffer Buffer
 * @param offset Offset where options start
 * @param len Length of buffer
 */
std::vector<std::pair<std::string, std::string>> parseOptions(char* buffer,
                                                              size_t offset,
                                                              size_t len) {
    std::vector<std::pair<std::string, std::string>> options;
    char* options_p = buffer + offset;
    while (options_p < buffer + len) {
        std::string option = std::string(options_p);
        options_p += option.length() + 1;
        std::string value = std::string(options_p);
        options_p += value.length() + 1;
        options.push_back(std::make_pair(option, value));
    }
    return options;
}

Packet parsePacket(char* buffer, size_t len) {
    Opcode opcode = (Opcode)ntohs(*(short*)buffer);
    switch (opcode) {
        case Opcode::RRQ: {
            RRQPacket rrq;
            rrq.filepath = std::string(buffer + 2);
            rrq.mode = std::string(buffer + 2 + rrq.filepath.length() + 1);
            size_t offset = 2 + rrq.filepath.length() + 1 + rrq.mode.length() + 1;
            rrq.options = parseOptions(buffer, offset, len);
            return rrq;
        }
        case Opcode::WRQ: {
            WRQPacket wrq;
            wrq.filepath = std::string(buffer + 2);
            wrq.mode = std::string(buffer + 2 + wrq.filepath.length() + 1);
            size_t offset = 2 + wrq.filepath.length() + 1 + wrq.mode.length() + 1;
            wrq.options = parseOptions(buffer, offset, len);
            return wrq;
        }
        case Opcode::DATA: {
            DATAPacket data;
            data.block = ntohs(*(short*)(buffer + 2));
            data.data = buffer + 4;
            data.len = len - 4;
            return data;
        }
        case Opcode::ACK: {
            ACKPacket ack;
            ack.block = ntohs(*(short*)(buffer + 2));
            return ack;
        }
        case Opcode::ERROR: {
            ERRORPacket error;
            error.code = (ErrorCode)ntohs(*(short*)(buffer + 2));
            error.message = std::string(buffer + 4);
            return error;
        }
        case Opcode::OACK: {
            OACKPacket oack;
            size_t offset = 2;
            oack.options = parseOptions(buffer, offset, len);
            return oack;
        }
        default:
            // Error handling
            break;
    }
}

/**
 * Get address string ({SRC_IP}:{SRC_PORT})
 * @param addr Address
 */
std::string getAddrString(sockaddr_in addr) {
    return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));
}

/**
 * Get address string + destination port ({SRC_IP}:{SRC_PORT}:{DST_PORT})
 * @param source Source address
 * @param dest Destination address
 */
std::string getAddrDstString(sockaddr_in source, sockaddr_in dest) {
    return getAddrString(source) + ":" + std::to_string(ntohs(dest.sin_port));
}

/**
 * Get options string ({OPT1_NAME}={OPT1_VALUE} ... {OPTn_NAME}={OPTn_VALUE})
 * @param options Options
 */
std::string getOptionsString(std::vector<std::pair<std::string, std::string>> options) {
    std::string str = "";
    for (auto& option : options) {
        str += " " + option.first + "=" + option.second;
    }
    return str;
}

void printPacket(Packet packet, sockaddr_in source, sockaddr_in dest) {
    std::visit(
        [source, dest](auto&& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, RRQPacket>) {
                std::cerr << "RRQ " << getAddrString(source) << " " << p.filepath << " " << p.mode
                          << getOptionsString(p.options) << std::endl;
            } else if constexpr (std::is_same_v<T, WRQPacket>) {
                std::cerr << "WRQ " << getAddrString(source) << " " << p.filepath << " " << p.mode
                          << getOptionsString(p.options) << std::endl;
            } else if constexpr (std::is_same_v<T, DATAPacket>) {
                std::cerr << "DATA " << getAddrDstString(source, dest) << " " << p.block
                          << std::endl;
            } else if constexpr (std::is_same_v<T, ACKPacket>) {
                std::cerr << "ACK " << getAddrString(source) << p.block << std::endl;
            } else if constexpr (std::is_same_v<T, ERRORPacket>) {
                std::cerr << "ERROR " << getAddrDstString(source, dest) << " " << (int)p.code
                          << " \"" << p.message << "\"" << std::endl;
            } else if constexpr (std::is_same_v<T, OACKPacket>) {
                std::cerr << "OACK " << getAddrString(source) << getOptionsString(p.options)
                          << std::endl;
            }
        },
        packet);
}
