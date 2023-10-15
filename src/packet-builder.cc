#include "packet-builder.h"
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include "enums.h"

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

char* PacketBuilder::getBuffer() {
    return this->buffer;
}

void PacketBuilder::createWRQ(std::string filepath, std::string mode) {
    this->resetPointer();
    this->addOpcode(Opcode::WRQ);
    this->addString(filepath);
    this->addString(mode);
}

void PacketBuilder::createRRQ(std::string filepath, std::string mode) {
    this->resetPointer();
    this->addOpcode(Opcode::RRQ);
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

void PacketBuilder::createACK(uint16_t block) {
    this->resetPointer();
    this->addOpcode(Opcode::ACK);
    this->add16b(block);
}

void PacketBuilder::createOACK() {
    this->resetPointer();
    this->addOpcode(Opcode::OACK);
}

void PacketBuilder::createERROR(ErrorCode code, std::string message) {
    this->resetPointer();
    this->addOpcode(Opcode::ERROR);
    this->add16b((uint16_t)code);
    this->addString(message);
}

void PacketBuilder::addBlksizeOption(size_t size) {
    this->addString("blksize");
    this->addString(std::to_string(size));
}

void PacketBuilder::addTimeoutOption(size_t time) {
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
