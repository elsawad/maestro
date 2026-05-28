#ifndef CHUNK_EXTENSIONS_H
#define CHUNK_EXTENSIONS_H

#include <optional>
#include <string>
#include <vector>

class ChunkExtensions {
  public:
    struct ChunkExtension {
      std::string name;
      std::optional<std::string> value;
    };

    bool empty() const;
  private:
    std::vector<ChunkExtension> chunk_extensions;
};

#endif
