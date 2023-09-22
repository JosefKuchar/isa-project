#include "client-args.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <charconv>
#include <iostream>
#include "enums.h"

ClientArgs::ClientArgs(int argc, char** argv) {
    int option, parsed_port;
    std::string hostname, port, source_file;
    bool hostname_set = false, port_set = false, source_file_set = false, dest_file_set = false;

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

    // Check if hostname is present
    if (!hostname_set) {
        std::cerr << "Hostname not set" << std::endl;
        exit(1);
    }

    // Check if destination filepath is present
    if (!dest_file_set) {
        std::cerr << "Destination filepath not set" << std::endl;
        exit(1);
    }

    // Parse address and check if it is valid
    if (inet_pton(AF_INET, hostname.c_str(), &this->address.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        exit(1);
    }

    // Parse port
    if (port_set) {
        auto res = std::from_chars(port.data(), port.data() + port.size(), parsed_port);
        if (res.ec != std::errc()) {
            std::cerr << "Invalid port" << std::endl;
            exit(1);
        }
    } else {
        parsed_port = DEFAULT_PORT;
    }

    // Parse source file
    if (source_file_set) {
        // Read from file
        this->input_file = fopen(source_file.c_str(), "r");
        if (this->input_file == nullptr) {
            std::cerr << "Error opening file" << std::endl;
            exit(1);
        }
    } else {
        // Read from stdin
        this->input_file = stdin;
    }

    this->address.sin_family = AF_INET;
    this->address.sin_port = htons(parsed_port);
}
