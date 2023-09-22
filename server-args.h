#pragma once

#include <netinet/in.h>
#include <fstream>
#include <string>

class ServerArgs {
   public:
    struct sockaddr_in address;
    std::string root_dirpath;

    ServerArgs(int argc, char** argv);
};
