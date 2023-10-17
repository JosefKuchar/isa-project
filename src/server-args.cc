/**
 * @author Josef Kuchař (xkucha28)
 */

#include "server-args.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <charconv>
#include <iostream>
#include "enums.h"

void ServerArgs::printHelp() {
    std::cout << "Usage: tftp-server [-p port] root_dirpath" << std::endl;
}

ServerArgs::ServerArgs(int argc, char** argv) {
    std::string port;
    std::string dirPath;
    int parsed_port;
    int port_set = true;

    // Check number of arguments
    if (argc != 2 && argc != 4) {
        this->printHelp();
        std::cout << "Invalid number of arguments" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Only root dir path specified
    if (argc == 2) {
        dirPath = argv[1];
        port_set = false;
    } else {
        // Port specified as first (and second) argument
        if (std::string(argv[1]) == "-p") {
            port = argv[2];
            dirPath = argv[3];
            // Port specified as second (and third) argument
        } else if (std::string(argv[2]) == "-p") {
            port = argv[3];
            dirPath = argv[1];
            // Invalid arguments
        } else {
            this->printHelp();
            std::cout << "Invalid arguments" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (port_set) {
        // Parse port
        auto res = std::from_chars(port.data(), port.data() + port.size(), parsed_port);
        if (res.ec != std::errc()) {
            this->printHelp();
            std::cout << "Invalid port" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        // Use default port
        parsed_port = DEFAULT_PORT;
    }

    // Parse path and check if it is a directory
    this->path = std::filesystem::canonical(dirPath);
    if (!std::filesystem::is_directory(this->path)) {
        this->printHelp();
        std::cout << "Invalid root directory path" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(parsed_port);
    this->address.sin_addr.s_addr = INADDR_ANY;
    this->len = sizeof(this->address);
}
