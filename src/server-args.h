/**
 * @author Josef Kuchař (xkucha28)
 */

#pragma once

#include <netinet/in.h>
#include <filesystem>
#include <fstream>
#include <string>

class ServerArgs {
   public:
    // Server address
    struct sockaddr_in address;
    // Server address length
    socklen_t len;
    // Server port
    std::filesystem::path path;

    ServerArgs(int argc, char** argv);
};
