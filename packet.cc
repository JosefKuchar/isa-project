#include "packet.h"
#include <arpa/inet.h>
#include <cstring>
#include <string>
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

void Packet::createDATA(uint16_t block, char* buffer, size_t len) {
    this->resetPointer();
    this->addOpcode(Opcode::DATA);
    this->add16b(block);
    // Copy data
    std::memcpy(this->buffer_p, buffer, len);
    this->buffer_p += len;
}

void Packet::addBlksizeOption(size_t size) {
    if (size < 8 || size > 65464) {
        // TODO: Throw error
    }

    this->addString("blksize");
    this->addString(std::to_string(size));
}

void Packet::addTimeoutOption(size_t time) {
    if (time < 1 || time > 255) {
        // TODO: Throw error
    }

    this->addString("timeout");
    this->addString(std::to_string(time));
}

void Packet::addTsizeOption(size_t tsize) {
    this->addString("tsize");
    this->addString(std::to_string(tsize));
}

Packet::Packet(char* buffer) {
    this->buffer = buffer;
}
