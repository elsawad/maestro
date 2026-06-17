#ifndef SERVER_CONFIG_BUILDER_H
#define SERVER_CONFIG_BUILDER_H

#include "ServerConfig.h"
#include "ServerConfigBuilderError.h"
#include "ServerConfigPatch.h"

class ServerConfigBuilder {
  public:
    ServerConfigBuilder & patch(ServerConfigPatch && patch);
    ServerConfigBuilderResult build() &&;
  private:
    std::optional<std::string> root{};
    std::optional<std::uint16_t> port{};
};

#endif
