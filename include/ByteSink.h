#ifndef BYTE_SINK_H
#define BYTE_SINK_H

#include <cstddef>

class ByteSink {
  public:
    virtual ~ByteSink() = default;
    virtual void write(const void *data, size_t size) = 0;
    virtual void finish() = 0;
};

#endif
