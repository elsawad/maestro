#ifndef CHUNKED_ENCODER_H
#define CHUNKED_ENCODER_H

#include "Encoder.h"

class ChunkedEncoder: public Encoder {
  public:
    explicit ChunkedEncoder(std::unique_ptr<ByteSink> byte_sink);
  private:
    void do_write(const void *data, size_t size) override;
    void do_finish() override;
};

#endif
