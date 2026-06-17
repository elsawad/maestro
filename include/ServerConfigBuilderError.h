#ifndef SERVER_CONFIG_BUILDER_ERROR_H
#define SERVER_CONFIG_BUILDER_ERROR_H

#include <expected>
#include <string>

struct ServerConfigBuilderError {
  enum class Type {
    MISSING_REQUIRED,
    INVALID_VALUE,
  };

  const Type type;
  const std::string message;
};

class ServerConfig;
using ServerConfigBuilderResult = std::expected<ServerConfig, ServerConfigBuilderError>;

#endif
