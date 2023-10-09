#pragma once

#include <netinet/in.h>
#include <filesystem>
#include <fstream>
#include <string>

class ServerArgs {
   public:
    struct sockaddr_in address;
    std::filesystem::path path;

    ServerArgs(int argc, char** argv);
};
