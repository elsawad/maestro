#include <charconv>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

#include "ServerConfigCLIParser.h"
#include "ServerConfigPatch.h"

std::expected<std::uint16_t, ServerConfigParserError> parse_port(const char * p) {
  if (p == nullptr) {
    return 0u;
  }

  const char * const end{p + std::strlen(p)};
  int port{0};

  const std::from_chars_result fcr{std::from_chars(p, end, port)};

  if (fcr.ec != std::errc{} || fcr.ptr != end) {
    return std::unexpected{ServerConfigParserError{ServerConfigParserError::Type::INVALID_VALUE, "invalid integer format"}};
  }

  if (port < std::numeric_limits<std::uint16_t>::min() || port > std::numeric_limits<std::uint16_t>::max()) {
    return std::unexpected{ServerConfigParserError{ServerConfigParserError::Type::INVALID_VALUE, "port must be between 0-65535"}};
  }

  return static_cast<std::uint16_t>(port);
}

ServerConfigParserResult ServerConfigCLIParser::parse(int argc, const char * const * argv) {
  const char * proot{nullptr};
  const char * pport{nullptr};

  std::size_t i{1};
  while (i < argc) {
    std::string_view option(argv[i]);

    if (option == "-r" && i + 1 == argc) {
      return std::unexpected{ServerConfigParserError{ServerConfigParserError::Type::MISSING_ARGUMENT, "-r requires path argument"}};
    }

    if (option == "-r") {
      proot = argv[i + 1];
      i += 2;
      continue;
    }

    if (option == "-p" && i + 1 == argc) {
      return std::unexpected{ServerConfigParserError{ServerConfigParserError::Type::MISSING_ARGUMENT, "-p requires port number argument"}};
    }

    if (option == "-p") {
      pport = argv[i + 1];
      i += 2;
      continue;
    }

    return std::unexpected{ServerConfigParserError{ServerConfigParserError::Type::UNKNOWN_OPTION, "unknown option: " + std::string{argv[i]}}};
  }

  ServerConfigPatch patch{};

  if (proot != nullptr) {
    patch.root = std::string{proot};
  }

  auto parse_port_result{parse_port(pport)};
  if (!parse_port_result) {
    return std::unexpected{parse_port_result.error()};
  }

  if (pport != nullptr) {
    patch.port = *parse_port_result;
  }

  return patch;
}
