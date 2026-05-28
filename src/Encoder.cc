#include <stdexcept>
#include <iostream>

#include "Encoder.h"

Encoder::Encoder(std::unique_ptr<ByteSink> byte_sink): byte_sink(std::move(byte_sink)) {
  if (!this->byte_sink) {
    throw std::invalid_argument("byte_sink cannot be null");
  }
}

void Encoder::write(const void *data, size_t size) {
  if (this->finished) {
    throw std::logic_error("write after finish");
  }

  if (data == nullptr && size > 0) {
    throw std::invalid_argument("data cannot be nullptr if size is non-zero");
  }

  this->do_write(data, size);
}

void Encoder::finish() {
  if (this->finished) {
    return;
  }

  this->finished = true;
  this->do_finish();
}
