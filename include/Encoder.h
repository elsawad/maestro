#ifndef ENCODER_H
#define ENCODER_H

#include <cstddef>
#include <memory>

#include "ByteSink.h"

class Encoder: public ByteSink {
  public:
    Encoder(std::unique_ptr<ByteSink> byte_sink);
    virtual ~Encoder() = default;
    virtual void write(const void *data, size_t size) override;
    virtual void finish() override;
  protected:
    const std::unique_ptr<ByteSink> byte_sink;
  private:
    bool finished{false};
    virtual void do_write(const void *data, size_t size) = 0;
    virtual void do_finish() = 0;
};

#endif
