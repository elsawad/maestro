#ifndef CHUNKED_BODY_H
#define CHUNKED_BODY_H

#include <vector>

#include "Chunk.h"
#include "FieldCollection.h"

class ChunkedBody {
  private:
    std::vector<Chunk> chunks;
    FieldCollection trailer_section;
};

#endif
