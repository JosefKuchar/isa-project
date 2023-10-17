/**
 * @author Josef Kuchař (xkucha28)
 */

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

    /*
     * Get packet buffer
     */
    char* getBuffer();

    /**
     * Create WRQ packet
     * @param filepath File path
     * @param mode Transfer mode
     */
    void createWRQ(std::string filepath, std::string mode);

    /**
     * Create RRQ packet
     * @param filepath File path
     * @param mode Transfer mode
     */
    void createRRQ(std::string filepath, std::string mode);

    /**
     * Create DATA packet
     * @param block Block number
     * @param buffer Data to be sent
     * @param len Length of data
     */
    void createDATA(uint16_t block, char* buffer, size_t len);

    /**
     * Create empty OACK packet
     */
    void createOACK();

    /**
     * Create ACK packet
     * @param block Block number
     */
    void createACK(uint16_t block);

    /**
     * Create ERROR packet
     * @param code Error code
     * @param message Error message
     */
    void createERROR(ErrorCode code, std::string message);

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
