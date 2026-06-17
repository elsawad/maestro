#include "ServerConfig.h"

ServerConfig::ServerConfig(std::filesystem::path && root, std::uint16_t port):
  root(std::move(root)),
  port(port) {}
