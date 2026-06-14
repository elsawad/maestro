#ifndef STREAM_PROCESSOR_H
#define STREAM_PROCESSOR_H

#include <cstdint>
#include <span>

#include "FeedResult.h"

class StreamProcessor {
  public:
    virtual ~StreamProcessor() = default;
    virtual FeedResult feed(std::span<const std::byte> span) = 0;
    virtual FeedState state() const = 0;
};

#endif
