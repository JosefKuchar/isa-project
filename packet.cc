#include "packet.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include "enums.h"

Packet parsePacket(char* buffer, size_t len) {
    Packet packet;
    Opcode opcode = (Opcode)ntohs(*(short*)buffer);
    switch (opcode) {
        case Opcode::RRQ: {
            RRQPacket rrq;
            rrq.filepath = std::string(buffer + 2);
            rrq.mode = std::string(buffer + 2 + rrq.filepath.length() + 1);
            packet = rrq;
            break;
        }
        case Opcode::WRQ: {
            WRQPacket wrq;
            wrq.filepath = std::string(buffer + 2);
            wrq.mode = std::string(buffer + 2 + wrq.filepath.length() + 1);
            packet = wrq;
            break;
        }
        case Opcode::DATA: {
            DATAPacket data;
            data.block = ntohs(*(short*)(buffer + 2));
            data.data = buffer + 4;
            data.len = len - 4;
            packet = data;
            break;
        }
        case Opcode::ACK: {
            ACKPacket ack;
            ack.block = ntohs(*(short*)(buffer + 2));
            packet = ack;
            break;
        }
        case Opcode::ERROR: {
            ERRORPacket error;
            error.code = (ErrorCode)ntohs(*(short*)(buffer + 2));
            error.message = std::string(buffer + 4);
            packet = error;
            break;
        }
        case Opcode::OACK: {
            OACKPacket oack;
            packet = oack;
            break;
        }
        default:
            // Error handling
            break;
    }
    return packet;
}

std::string getAddrString(sockaddr_in addr) {
    return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));
}

std::string getAddrStringDst(sockaddr_in source, sockaddr_in dest) {
    return getAddrString(source) + ":" + std::to_string(ntohs(dest.sin_port));
}

void printPacket(Packet packet, sockaddr_in source, sockaddr_in dest) {
    std::visit(
        [source, dest](auto&& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, RRQPacket>) {
                std::cerr << "RRQ " << getAddrString(source) << " " << p.filepath << " " << p.mode
                          << std::endl;
            } else if constexpr (std::is_same_v<T, WRQPacket>) {
                std::cerr << "WRQ " << getAddrString(source) << " " << p.filepath << " " << p.mode
                          << std::endl;
            } else if constexpr (std::is_same_v<T, DATAPacket>) {
                std::cerr << "DATA " << getAddrStringDst(source, dest) << " " << p.block
                          << std::endl;
            } else if constexpr (std::is_same_v<T, ACKPacket>) {
                std::cerr << "ACK " << getAddrString(source) << p.block << std::endl;
            } else if constexpr (std::is_same_v<T, ERRORPacket>) {
                std::cerr << "ERROR " << getAddrStringDst(source, dest) << " " << (int)p.code
                          << " \"" << p.message << "\"" << std::endl;
            } else if constexpr (std::is_same_v<T, OACKPacket>) {
                std::cout << "OACK " << getAddrString(source) << std::endl;
            }
        },
        packet);
}
