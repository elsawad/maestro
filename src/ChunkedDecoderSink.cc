#include <cstddef>
#include <span>

#include "ChunkedDecoderSink.h"

ChunkedDecoderSink::ChunkedDecoderSink(std::unique_ptr<ByteSink> byte_sink): byte_sink(std::move(byte_sink)) {
  this->decoder.set_handler(*this);
}

void ChunkedDecoderSink::write(const void *data, size_t size) {
  this->decoder.feed(std::span<const std::byte>{static_cast<const std::byte *>(data), size});
}

void ChunkedDecoderSink::finish() {
  // We will ignore any remaining data in the decoder since the decoder will have already consumed all valid chunks and trailer sections from the input
  if (this->decoder.state() != FeedState::COMPLETE) {
    throw std::runtime_error("Cannot finish ChunkedDecoderSink: decoder is in progress or invalid");
  }
  this->byte_sink->finish();
}

void ChunkedDecoderSink::on_data(std::span<const std::byte> data) {
  this->byte_sink->write(data.data(), data.size());
}

void ChunkedDecoderSink::on_finish() {
  this->byte_sink->finish();
}
