#ifndef CHUNK_EXTENSION_H
#define CHUNK_EXTENSION_H

#include <optional>
#include <string>

struct ChunkExtension {
  std::string name;
  std::optional<std::string> val;
};

#endif
