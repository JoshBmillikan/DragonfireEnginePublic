//
// Created by josh on 2/13/23.
//
#include "server.h"

int main(int argc, char** argv)
{
    df::Server server(argc, argv);
    server.run();
    return 0;
}