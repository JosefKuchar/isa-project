#include "packet.h"
#include <arpa/inet.h>
#include "enums.h"

// TODO Check lengths - buffer overflow

/**
 * Add uint16_t to packet
 * @param data 16b data
 */
void Packet::add16b(uint16_t data) {
    *(short*)(this->buffer_p) = htons(data);
    this->buffer_p += 2;
}

/**
 * Add packet opcode to packet
 * @param opcode Opcode
 */
void Packet::addOpcode(Opcode opcode) {
    this->add16b((uint16_t)opcode);
}

/**
 * Add string to packet (including terminal 0)
 * @param str String
 */
void Packet::addString(std::string str) {
    str.copy(this->buffer_p, str.length());
    this->buffer_p += str.length() + 1;
}

/**
 * Reset pointer back to buffer start
 */
void Packet::resetPointer() {
    this->buffer_p = &this->buffer[0];
}

size_t Packet::getSize() {
    return this->buffer_p - this->buffer;
}

void Packet::createWRQ(std::string filepath, std::string mode) {
    this->resetPointer();
    this->addOpcode(Opcode::WRQ);
    this->addString(filepath);
    this->addString(mode);
}

void Packet::createDATA(uint16_t block) {
    this->resetPointer();
    this->addOpcode(Opcode::DATA);
    this->add16b(block);
}

Packet::Packet(char* buffer) {
    this->buffer = buffer;
}
