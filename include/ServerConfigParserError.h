#ifndef SERVER_CONFIG_PARSER_ERROR_H
#define SERVER_CONFIG_PARSER_ERROR_H

#include <expected>
#include <string>

struct ServerConfigParserError {
  enum class Type {
    MISSING_ARGUMENT,
    INVALID_VALUE,
    UNKNOWN_OPTION,
  };

  const Type type;
  const std::string message;
};

class ServerConfigPatch;

using ServerConfigParserResult = std::expected<ServerConfigPatch, ServerConfigParserError>;

#endif
