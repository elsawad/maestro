#ifndef BASIC_OSTREAM_SINK_H
#define BASIC_OSTREAM_SINK_H

#include <ostream>

#include "ByteSink.h"

template <typename T>
class BasicOstreamSink: public ByteSink {
  public:
    explicit BasicOstreamSink(std::basic_ostream<T> &os);
    void write(const void *data, size_t size) override;
    void finish() override;
  private:
    std::basic_ostream<T> &os;
};

#include "BasicOstreamSink.tcc"

#endif
