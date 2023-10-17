#pragma once

#include <netinet/in.h>
#include <fstream>
#include <string>

class ClientArgs {
   public:
    // Server address
    struct sockaddr_in address;
    // Server address length
    socklen_t len;
    // Server port
    in_port_t port;
    // Input file
    FILE* input_file;
    // Destination file path
    std::string dest_filepath;
    // Whether to send or recieve
    bool send;

    ClientArgs(int argc, char** argv);
};
