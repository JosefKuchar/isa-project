#include "utils.h"
#include <string.h>
#include <iostream>
#include "enums.h"

void print_rrq_wrq(char* buffer) {
    std::cerr << " \"" << buffer + 2 << "\" ";
    std::cerr << buffer + 2 + strlen(buffer + 2) + 1;
}

void print_addr(sockaddr_in addr) {
    std::cerr << " " << inet_ntoa(addr.sin_addr) << ":";
    std::cerr << ntohs(addr.sin_port);
}

void print_block(char* buffer) {
    std::cerr << " " << ntohs(*(short*)(buffer + 2));
}

void print_packet(char* buffer, sockaddr_in addr) {
    uint16_t opcode = ntohs(*(uint16_t*)buffer);

    switch (opcode) {
        case (uint16_t)Opcode::RRQ:
            std::cerr << "RRQ";
            print_addr(addr);
            print_rrq_wrq(buffer);
            break;
        case (uint16_t)Opcode::WRQ:
            std::cerr << "WRQ";
            print_addr(addr);
            print_rrq_wrq(buffer);
            break;
        case (uint16_t)Opcode::DATA:
            std::cerr << "DATA";
            print_addr(addr);
            print_block(buffer);
            break;
        case (uint16_t)Opcode::ACK:
            std::cerr << "ACK";
            print_addr(addr);
            print_block(buffer);
            break;
        case (uint16_t)Opcode::ERROR:
            std::cerr << "ERROR";
            print_addr(addr);
            break;
        case (uint16_t)Opcode::OACK:
            std::cerr << "OACK";
            print_addr(addr);
            break;
    }
    std::cerr << std::endl;
}
