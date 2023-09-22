#pragma once

#include <string>
#include "enums.h"

class Packet {
   public:
    Packet(char* buffer);

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
     */
    void createDATA(uint16_t block);

   private:
    void add16b(uint16_t data);
    void addOpcode(Opcode opcode);
    void addString(std::string str);
    void resetPointer();

    char* buffer;
    char* buffer_p;
};
