#pragma once

#include <string>
#include "enums.h"

class PacketBuilder {
   public:
    PacketBuilder(char* buffer);

    /**
     * Get packet size
     */
    size_t getSize();

    /**
     * Create WRQ packet
     * @param filepath File path
     * @param mode Transfer mode
     */
    void createWRQ(std::string filepath, std::string mode);

    /**
     * Create DATA packet
     * @param block Block number
     * @param buffer Data to be sent
     * @param len Length of data
     */
    void createDATA(uint16_t block, char* buffer, size_t len);

    /**
     * Add Blksize option to packet
     * @param size Block size
     */
    void addBlksizeOption(size_t size);

    /**
     * Add Timeout option to packet
     * @param time Timeout in secs
     */
    void addTimeoutOption(size_t time);

    /**
     * Add Transfer size option to packet
     * @param tsize Transfer size in octets
     */
    void addTsizeOption(size_t tsize);

   private:
    void add16b(uint16_t data);
    void addOpcode(Opcode opcode);
    void addString(std::string str);
    void resetPointer();

    char* buffer;
    char* buffer_p;
};
