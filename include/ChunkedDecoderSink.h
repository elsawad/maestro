#ifndef CHUNKED_DECODER_SINK_H
#define CHUNKED_DECODER_SINK_H

#include <memory>

#include "ByteSink.h"
#include "ChunkedDecoder.h"

class ChunkedDecoderSink: public ByteSink, public ChunkedDecoder::Handler {
  public:
    explicit ChunkedDecoderSink(std::unique_ptr<ByteSink> byte_sink);
    void write(const void *data, size_t size) override;
    void finish() override;
    void on_data(std::span<const std::byte> data) override;
    void on_finish() override;

  private:
    std::unique_ptr<ByteSink> byte_sink;
    ChunkedDecoder decoder;
};

#endif
