//
// Created by josh on 2/13/23.
//

#include "server.h"
#include "file.h"
#include <nlohmann/json.hpp>

namespace df {
Server::Server(int argc, char** argv) : BaseGame(argc, argv)
{
    loadServerConfig();
    connections.reserve(config.maxPlayerCount);
}

void Server::update(double deltaSeconds)
{
}

Server::~Server()
{
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Server::Config, port, maxPlayerCount);

void Server::loadServerConfig()
{
    try {
        File file("server.json");
        nlohmann::json json = nlohmann::json::parse(file.readToString(), nullptr, true, true);
        config = json.get<Config>();
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load server config: {}", e.what());
    }
}

}   // namespace df