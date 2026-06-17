#ifndef SERVER_CONFIG_PATCH_H
#define SERVER_CONFIG_PATCH_H

#include <cstdint>
#include <optional>
#include <string>

struct ServerConfigPatch {
  std::optional<std::string> root{};
  std::optional<std::uint16_t> port{};
};

#endif
