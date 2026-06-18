#ifndef CODING_PIPELINE_BUILDER_H
#define CODING_PIPELINE_BUILDER_H

#include "HTTPMessageHeaders.h"

class CodingPipelineBuilder {
  public:
    explicit CodingPipelineBuilder(HTTPMessageHeaders &headers);
};

#endif
