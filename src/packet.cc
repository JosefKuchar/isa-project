#include "packet.h"
#include <arpa/inet.h>
#include <algorithm>
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
std::optional<Options> parseOptionsToStruct(std::vector<std::pair<std::string, std::string>> opts) {
    if (opts.size() == 0) {
        return std::nullopt;
    }

    Options options;
    options.valid = true;
    for (auto& [option, value] : opts) {
        // Check if value is a number
        try {
            int valueNumber = std::stoi(value);

            // To lower
            std::transform(option.begin(), option.end(), option.begin(), ::tolower);

            if (option == "blksize") {
                options.blkSize = valueNumber;

                if (options.blkSize < 8 && options.blkSize > 65464) {
                    std::cout << "Invalid block size: " << options.blkSize.value() << std::endl;
                    options.valid = false;
                    break;
                }
            } else if (option == "timeout") {
                options.timeout = valueNumber;

                if (options.timeout < 0 && options.timeout > 255) {
                    std::cout << "Invalid timeout: " << options.timeout.value() << std::endl;
                    options.valid = false;
                    break;
                }
            } else if (option == "tsize") {
                options.tSize = valueNumber;

                if (options.tSize < 0) {
                    std::cout << "Invalid tsize: " << options.tSize.value() << std::endl;
                    options.valid = false;
                    break;
                }
            } else {
                std::cout << "Unknown option: " << option << std::endl;
                options.valid = false;
                break;
            }
        } catch (std::invalid_argument& e) {
            std::cout << "Invalid value: " << value << std::endl;
            options.valid = false;
            break;
        }
    }

    return std::optional<Options>{options};
}

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
    }

    return UnknownPacket();
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

void printPacket(Packet packet, sockaddr_in source, sockaddr_in dest, bool debug) {
    std::string msg = std::visit(
        [source, dest](auto&& p) {
            using T = std::decay_t<decltype(p)>;
            if constexpr (std::is_same_v<T, RRQPacket>) {
                return "RRQ " + getAddrString(source) + " " + p.filepath + " " + p.mode +
                       getOptionsString(p.options);
            } else if constexpr (std::is_same_v<T, WRQPacket>) {
                return "WRQ " + getAddrString(source) + " " + p.filepath + " " + p.mode +
                       getOptionsString(p.options);
            } else if constexpr (std::is_same_v<T, DATAPacket>) {
                return "DATA " + getAddrDstString(source, dest) + " " + std::to_string(p.block);
            } else if constexpr (std::is_same_v<T, ACKPacket>) {
                return "ACK " + getAddrString(source) + " " + std::to_string(p.block);
            } else if constexpr (std::is_same_v<T, ERRORPacket>) {
                return "ERROR " + getAddrDstString(source, dest) + " " +
                       std::to_string((int)p.code) + " \"" + p.message + "\"";
            } else if constexpr (std::is_same_v<T, OACKPacket>) {
                return "OACK " + getAddrString(source) + getOptionsString(p.options);
            } else {
                return std::string("Unknown packet");
            }
        },
        packet);
    if (debug) {
        std::cout << ">> " << msg << std::endl;
    } else {
        std::cerr << msg << std::endl;
    }
}
