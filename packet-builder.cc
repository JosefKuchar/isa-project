#include "packet-builder.h"
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include "enums.h"

// TODO Check lengths - buffer overflow

/**
 * Add uint16_t to packet
 * @param data 16b data
 */
void PacketBuilder::add16b(uint16_t data) {
    *(short*)(this->buffer_p) = htons(data);
    this->buffer_p += 2;
}

/**
 * Add packet opcode to packet
 * @param opcode Opcode
 */
void PacketBuilder::addOpcode(Opcode opcode) {
    this->add16b((uint16_t)opcode);
}

/**
 * Add string to packet (including terminal 0)
 * @param str String
 */
void PacketBuilder::addString(std::string str) {
    str.copy(this->buffer_p, str.length());
    this->buffer_p[str.length()] = 0;
    this->buffer_p += str.length() + 1;
}

/**
 * Reset pointer back to buffer start
 */
void PacketBuilder::resetPointer() {
    this->buffer_p = &this->buffer[0];
}

size_t PacketBuilder::getSize() {
    return this->buffer_p - this->buffer;
}

void PacketBuilder::createWRQ(std::string filepath, std::string mode) {
    this->resetPointer();
    this->addOpcode(Opcode::WRQ);
    this->addString(filepath);
    this->addString(mode);
}

void PacketBuilder::createDATA(uint16_t block, char* buffer, size_t len) {
    this->resetPointer();
    this->addOpcode(Opcode::DATA);
    this->add16b(block);
    // Copy data
    std::memcpy(this->buffer_p, buffer, len);
    this->buffer_p += len;
}

void PacketBuilder::addBlksizeOption(size_t size) {
    if (size < 8 || size > 65464) {
        // TODO: Throw error
    }

    this->addString("blksize");
    this->addString(std::to_string(size));
}

void PacketBuilder::addTimeoutOption(size_t time) {
    if (time < 1 || time > 255) {
        // TODO: Throw error
    }

    this->addString("timeout");
    this->addString(std::to_string(time));
}

void PacketBuilder::addTsizeOption(size_t tsize) {
    this->addString("tsize");
    this->addString(std::to_string(tsize));
}

PacketBuilder::PacketBuilder(char* buffer) {
    this->buffer = buffer;
}
