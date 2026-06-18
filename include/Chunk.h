#ifndef CHUNK_H
#define CHUNK_H

#include <vector>

#include "ChunkExtensions.h"

class Chunk {
  public:
    ChunkExtensions extensions;
    std::vector<std::byte> data;
};

#endif
