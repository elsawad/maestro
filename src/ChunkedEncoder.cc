#include <string>
#include <sstream>

#include "ChunkedEncoder.h"

const std::string crlf{"\r\n"};
const std::string last_chunk_str{"0\r\n"};

ChunkedEncoder::ChunkedEncoder(std::unique_ptr<ByteSink> byte_sink): Encoder(std::move(byte_sink)) {}

void ChunkedEncoder::do_write(const void *data, size_t size) {
  if (size == 0) {
    return;
  }

  std::stringstream ss;
  ss << std::uppercase << std::hex << size << crlf;
  ss.write(static_cast<const char *>(data), size);
  ss << crlf;

  const std::string chunk_str{ss.str()};
  this->byte_sink->write(chunk_str.data(), chunk_str.size());
}

void ChunkedEncoder::do_finish() {
  this->byte_sink->write(last_chunk_str.data(), last_chunk_str.size());
}
