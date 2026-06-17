#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <filesystem>

class ServerConfig {
  public:
    const std::filesystem::path root;
    const std::uint16_t port;
  private:
    friend class ServerConfigBuilder;
    ServerConfig(std::filesystem::path && root, std::uint16_t port);
};

#endif
