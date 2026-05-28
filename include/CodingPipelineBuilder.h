#ifndef CODING_PIPELINE_BUILDER_H
#define CODING_PIPELINE_BUILDER_H

#include <memory>

#include "ByteSink.h"
#include "HTTPMessageHeaders.h"

class CodingPipelineBuilder {
  public:
    explicit CodingPipelineBuilder(HTTPMessageHeaders &headers);
};

#endif
