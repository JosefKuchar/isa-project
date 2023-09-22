#pragma once

#include <netinet/in.h>
#include <fstream>
#include <string>

class ClientArgs {
   public:
    struct sockaddr_in address;
    FILE* input_file;
    std::string dest_filepath;

    ClientArgs(int argc, char** argv);
};
