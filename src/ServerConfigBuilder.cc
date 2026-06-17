#include "ServerConfigBuilder.h"

ServerConfigBuilder & ServerConfigBuilder::patch(ServerConfigPatch && patch) {
  if (patch.port != std::nullopt) {
    this->port = patch.port;
  }

  if (patch.root != std::nullopt) {
    this->root = std::move(patch.root);
  }

  return *this;
}

ServerConfigBuilderResult ServerConfigBuilder::build() && {
  if (this->root == std::nullopt || this->root->empty()) {
    return std::unexpected{ServerConfigBuilderError{ServerConfigBuilderError::Type::MISSING_REQUIRED, "root is a required argument"}};
  }

  std::filesystem::path fsp_root{*this->root};
  if (!std::filesystem::is_directory(fsp_root)) {
    return std::unexpected{ServerConfigBuilderError{ServerConfigBuilderError::Type::INVALID_VALUE, "root is not a directory"}};
  }

  if (this->port == std::nullopt) {
    return std::unexpected{ServerConfigBuilderError{ServerConfigBuilderError::Type::MISSING_REQUIRED, "port is a required argument"}};
  }

  return ServerConfig{std::move(fsp_root), *this->port};
}
