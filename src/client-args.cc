/**
 * @author Josef Kuchař (xkucha28)
 */

#include "client-args.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <charconv>
#include <iostream>
#include "enums.h"

void ClientArgs::printHelp() {
    std::cout << "Usage: tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath"
              << std::endl;
}

ClientArgs::ClientArgs(int argc, char** argv) {
    int option, parsed_port;
    std::string hostname, port, source_file;
    struct addrinfo hints = {}, *addrs;
    bool hostname_set = false, port_set = false, source_file_set = false, dest_file_set = false;
    char port_str[16] = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // Parse command line arguments
    while ((option = getopt(argc, argv, "h:p:f:t:")) != -1) {
        switch (option) {
            case 'h':
                hostname = optarg;
                hostname_set = true;
                break;
            case 'p':
                port = optarg;
                port_set = true;
                break;
            case 'f':
                source_file = optarg;
                source_file_set = true;
                break;
            case 't':
                this->dest_filepath = optarg;
                dest_file_set = true;
                break;
        }
    }

    if (source_file_set && dest_file_set) {
        // Swap source and destination
        std::string tmp = this->dest_filepath;
        this->dest_filepath = source_file;
        source_file = tmp;
    }

    // Check if hostname is present
    if (!hostname_set) {
        this->printHelp();
        std::cout << "Hostname not set" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Check if destination filepath is present
    if (!dest_file_set) {
        this->printHelp();
        std::cout << "Destination filepath not set" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parse address and check if it is valid
    if (getaddrinfo(hostname.c_str(), port_str, &hints, &addrs) != 0) {
        this->printHelp();
        std::cout << "Invalid hostname" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Parse port
    if (port_set) {
        auto res = std::from_chars(port.data(), port.data() + port.size(), parsed_port);
        if (res.ec != std::errc()) {
            this->printHelp();
            std::cout << "Invalid port" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        parsed_port = DEFAULT_PORT;
    }

    // Parse source file
    if (source_file_set) {
        // Write to file
        this->input_file = fopen(source_file.c_str(), "wb");
        this->input_filepath = source_file;
        if (this->input_file == nullptr) {
            this->printHelp();
            std::cout << "Error opening file" << std::endl;
            exit(EXIT_FAILURE);
        }
        // This means we are downloading a file
        this->send = false;
    } else {
        // Read from stdin
        this->input_file = stdin;
        // This means we are uploading a file
        this->send = true;
    }

    // Set address
    for (auto addr = addrs; addr != nullptr; addr = addr->ai_next) {
        if (addr->ai_family == AF_INET) {
            this->address = *(struct sockaddr_in*)addr->ai_addr;
            this->address.sin_port = htons(parsed_port);
            break;
        }
    }

    // Free address info
    freeaddrinfo(addrs);

    // Set port
    this->port = htons(parsed_port);

    // Set len
    this->len = sizeof(this->address);
}
